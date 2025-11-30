#include "pch.h"
#include "DepthOfFieldPass.h"
#include "SceneView.h"
#include "SwapGuard.h"
#include "DepthOfFieldComponent.h"

void FDepthOfFieldPass::Execute(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
	if (UDepthOfFieldComponent* DofComponent = Cast<UDepthOfFieldComponent>(M.SourceObject); DofComponent == nullptr)
	{
		UE_LOG("DepthOfField: Source object is not a DepthOfFieldComponent!\n");
		return;
	}

	// ============================================================
	// (1) CoC 단계
	// ============================================================

	// [선택] A. Downscale
	//ExecuteDownscalePass(View, RHIDevice);

	// [필수] B. CoC 계산
	ExecuteCoCPass(M, View, RHIDevice);


	// ============================================================
	// (2) Blur 단계
	// ============================================================

	// [선택] C. Prefilter
	//ExecutePreFilterPass(View, RHIDevice);

	// [선택] D. Near/Far Split
	//ExecuteNearFarSplitPass(View, RHIDevice);

	// [선택] E. Dilation
	ExecuteDilationPass(View, RHIDevice);

	// [필수] F. Hex Blur (3-pass)
	ExecuteHexBlurPass1(View, RHIDevice);
	ExecuteHexBlurPass2(View, RHIDevice);
	ExecuteHexBlurPass3(View, RHIDevice);

	// [선택] G. Upscale
	//ExecuteUpscalePass(View, RHIDevice);

	// [선택] T. Temporal Blend
	//ExecuteTemporalBlendPass(View, RHIDevice);


	// ============================================================
	// (3) Composite 단계
	// ============================================================
	// 
	// [필수] H. Composite
	ExecuteCompositePass(M, View, RHIDevice);
}

void FDepthOfFieldPass::ExecuteDownscalePass(FSceneView* View, D3D11RHI* RHIDevice)
{
	FSwapGuard Swap(RHIDevice, 0, 2);
	RHIDevice->OMSetRenderTargets(ERTVMode::DofBlurTarget);

	UShader* VS = UResourceManager::GetInstance().Load<UShader>("FullScreenTriangle_VS");
	UShader* PS = UResourceManager::GetInstance().Load<UShader>("DoF_Downscale_PS");
	RHIDevice->PrepareShader(VS, PS);

	ID3D11ShaderResourceView* SRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &SRV);
	RHIDevice->DrawFullScreenQuad();

	Swap.Commit();
}

void FDepthOfFieldPass::ExecuteCoCPass(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
	// CoC 계산: 깊이 버퍼를 읽어서 Circle of Confusion 값을 계산하고 DofCoCTarget에 저장
	UDepthOfFieldComponent* DofComponent = Cast<UDepthOfFieldComponent>(M.SourceObject);
	if (!DofComponent) { return; }

	// 이전 패스의 RTV/SRV 완전히 언바인드
	ID3D11RenderTargetView* nullRTV = nullptr;
	RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, nullSRVs);

	// 1) 스왑 + SRV 언바인드 관리
	FSwapGuard Swap(RHIDevice, /*FirstSlot*/0, /*NumSlotsToUnbind*/2);

	// 2) DofCoC 타깃으로 렌더링
	RHIDevice->OMSetRenderTargets(ERTVMode::DofCoCTarget);

	// Depth State: Depth Test/Write 모두 OFF
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	// 3) 셰이더 로드
	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* CoCPS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DoF_CoC_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !CoCPS || !CoCPS->GetPixelShader())
	{
		UE_LOG("DoF CoC 셰이더 없음!\n");
		return;
	}

	RHIDevice->PrepareShader(FullScreenTriangleVS, CoCPS);

	// 4) SRV/Sampler (깊이 + 씬 컬러)
	ID3D11ShaderResourceView* DepthSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneDepth);
	ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
	ID3D11SamplerState* PointClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);
	ID3D11SamplerState* LinearClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);

	if (!DepthSRV || !SceneSRV || !PointClampSampler || !LinearClampSampler)
	{
		UE_LOG("DoF CoC: Required SRVs or Samplers are null!\n");
		return;
	}

	ID3D11ShaderResourceView* Srvs[2] = { DepthSRV, SceneSRV };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, Srvs);

	ID3D11SamplerState* Smps[2] = { PointClampSampler, LinearClampSampler };
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 2, Smps);

	// 5) 상수 버퍼 업데이트
	ECameraProjectionMode ProjectionMode = View->ProjectionMode;
	RHIDevice->SetAndUpdateConstantBuffer(PostProcessBufferType(View->NearClip, View->FarClip, ProjectionMode == ECameraProjectionMode::Orthographic));

	// DoF 설정 가져오기
	FDepthOfFieldSettings DofSettings = DofComponent->GetDepthOfFieldSettings();
	FDepthOfFieldBufferType DofBuffer;
	DofBuffer.FocalDistance = DofSettings.FocalDistance;
	DofBuffer.FocalLength = DofSettings.FocalLength;
	DofBuffer.FNumber = DofSettings.Aperture;
	DofBuffer.MaxCoc = DofSettings.MaxCocRadius;
	DofBuffer.NearTransitionRange = DofSettings.NearTransitionRange;
	DofBuffer.FarTransitionRange = DofSettings.FarTransitionRange;
	DofBuffer.NearBlurScale = DofSettings.NearBlurScale;
	DofBuffer.FarBlurScale = DofSettings.FarBlurScale;
	DofBuffer.BlurSampleCount = DofSettings.BlurSampleCount;
	DofBuffer.BokehRotation = DofSettings.BokehRotationRadians;
	DofBuffer.Weight = M.Weight;
	DofBuffer._Pad = 0.0f;

	RHIDevice->SetAndUpdateConstantBuffer(DofBuffer);

	// 6) Draw
	RHIDevice->DrawFullScreenQuad();

	// 7) 확정
	Swap.Commit();
}

void FDepthOfFieldPass::ExecutePreFilterPass(FSceneView* View, D3D11RHI* RHIDevice)
{
}

void FDepthOfFieldPass::ExecuteNearFarSplitPass(FSceneView* View, D3D11RHI* RHIDevice)
{
}

void FDepthOfFieldPass::ExecuteDilationPass(FSceneView* View, D3D11RHI* RHIDevice)
{
	// Dilation Pass: CoC 맵을 확장하여 전경 영역 보호
	// Min Filter를 적용하여 작은 CoC(전경)를 주변으로 퍼뜨림

	// 1) CoC 맵을 임시 버퍼(DofBlurTarget)로 복사
	{
		FSwapGuard Swap(RHIDevice, 0, 1);
		RHIDevice->OMSetRenderTargets(ERTVMode::DofBlurTarget);
		RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
		RHIDevice->OMSetBlendState(false);

		UShader* VS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
		UShader* PS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DoF_Dilation_PS.hlsl");

		if (!VS || !VS->GetVertexShader() || !PS || !PS->GetPixelShader())
		{
			UE_LOG("DoF Dilation 셰이더 없음!\n");
			return;
		}

		RHIDevice->PrepareShader(VS, PS);

		// CoC 맵을 입력으로 사용 (Point Sampler로 정확한 픽셀 값 샘플링)
		ID3D11ShaderResourceView* CoCMapSRV = RHIDevice->GetSRV(RHI_SRV_Index::DofCocMap);
		ID3D11SamplerState* PointClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);

		if (!CoCMapSRV || !PointClampSampler)
		{
			UE_LOG("DoF Dilation: CoC Map SRV or Sampler is null!\n");
			return;
		}

		RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &CoCMapSRV);
		RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &PointClampSampler);

		RHIDevice->DrawFullScreenQuad();
		Swap.Commit();
	}

	// 2) Dilated 결과를 다시 CoC 맵으로 복사 (단순 1:1 복사, Dilation 재적용 X)
	{
		ID3D11RenderTargetView* nullRTV = nullptr;
		RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);

		ID3D11ShaderResourceView* nullSRV = nullptr;
		RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &nullSRV);

		FSwapGuard Swap(RHIDevice, 0, 1);
		RHIDevice->OMSetRenderTargets(ERTVMode::DofCoCTarget);
		RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
		RHIDevice->OMSetBlendState(false);

		UShader* VS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
		// 단순 1:1 복사 (Passthrough) - Dilation 재적용하지 않음
		UShader* PS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/Passthrough_PS.hlsl");

		if (!VS || !VS->GetVertexShader() || !PS || !PS->GetPixelShader()) { return; }

		RHIDevice->PrepareShader(VS, PS);

		// DofBlurMap(임시 버퍼)를 입력으로 사용
		ID3D11ShaderResourceView* BlurMapSRV = RHIDevice->GetSRV(RHI_SRV_Index::DofBlurMap);
		ID3D11SamplerState* PointClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::PointClamp);

		RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 1, &BlurMapSRV);
		RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &PointClampSampler);

		RHIDevice->DrawFullScreenQuad();
		Swap.Commit();
	}
}

void FDepthOfFieldPass::ExecuteHexBlurPass1(FSceneView* View, D3D11RHI* RHIDevice)
{
	// 이전 패스의 RTV/SRV 완전히 언바인드
	ID3D11RenderTargetView* nullRTV = nullptr;
	RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, nullSRVs);

	// SwapGuard가 SwapRenderTargets()를 호출
	FSwapGuard Swap(RHIDevice, 0, 2);

	// 새로운 RTV 설정
	RHIDevice->OMSetRenderTargets(ERTVMode::DofBlurTarget);

	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	UShader* VS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* PS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DoF_HexBlur1_PS.hlsl");
	if (!VS || !VS->GetVertexShader() || !PS || !PS->GetPixelShader())
	{
		UE_LOG("DoF HexBlur1 셰이더 없음!\n");
		return;
	}

	RHIDevice->PrepareShader(VS, PS);

	ID3D11ShaderResourceView* Srvs[2] = {
		RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource),
		RHIDevice->GetSRV(RHI_SRV_Index::DofCocMap)
	};
	ID3D11SamplerState* LinearClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
	if (!Srvs[0] || !Srvs[1] || !LinearClampSampler)
	{
		UE_LOG("DoF HexBlur1: Required SRVs or Samplers are null!\n");
		return;
	}

	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, Srvs);
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &LinearClampSampler);

	RHIDevice->DrawFullScreenQuad();

	Swap.Commit();
}

void FDepthOfFieldPass::ExecuteHexBlurPass2(FSceneView* View, D3D11RHI* RHIDevice)
{
	// 이전 패스의 RTV/SRV 완전히 언바인드
	ID3D11RenderTargetView* nullRTV = nullptr;
	RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, nullSRVs);

	// SwapGuard가 SwapRenderTargets()를 호출
	FSwapGuard Swap(RHIDevice, 0, 2);

	// DofCoCTarget을 덮어쓰면 안됨! SceneColorTarget을 임시 타겟으로 활용
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	UShader* VS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* PS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DoF_HexBlur2_PS.hlsl");
	if (!VS || !VS->GetVertexShader() || !PS || !PS->GetPixelShader())
	{
		UE_LOG("DoF HexBlur2 셰이더 없음!\n");
		return;
	}

	RHIDevice->PrepareShader(VS, PS);

	ID3D11ShaderResourceView* Srvs[2] = {
		RHIDevice->GetSRV(RHI_SRV_Index::DofBlurMap), // Pass1의 결과
		RHIDevice->GetSRV(RHI_SRV_Index::DofCocMap)
	};
	ID3D11SamplerState* LinearClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
	if (!Srvs[0] || !Srvs[1] || !LinearClampSampler)
	{
		UE_LOG("DoF HexBlur2: Required SRVs or Samplers are null!\n");
		return;
	}

	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, Srvs);
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &LinearClampSampler);

	RHIDevice->DrawFullScreenQuad();

	Swap.Commit();
}

void FDepthOfFieldPass::ExecuteHexBlurPass3(FSceneView* View, D3D11RHI* RHIDevice)
{
	// 이전 패스의 RTV/SRV 완전히 언바인드
	ID3D11RenderTargetView* nullRTV = nullptr;
	RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);

	ID3D11ShaderResourceView* nullSRVs[2] = { nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, nullSRVs);

	// SwapGuard가 SwapRenderTargets()를 호출
	FSwapGuard Swap(RHIDevice, 0, 2);

	// 새로운 RTV 설정
	RHIDevice->OMSetRenderTargets(ERTVMode::DofBlurTarget);

	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	UShader* VS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* PS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DoF_HexBlur3_PS.hlsl");
	if (!VS || !VS->GetVertexShader() || !PS || !PS->GetPixelShader())
	{
		UE_LOG("DoF HexBlur3 셰이더 없음!\n");
		return;
	}

	RHIDevice->PrepareShader(VS, PS);

	ID3D11ShaderResourceView* Srvs[2] = {
		RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource), // Pass2의 결과
		RHIDevice->GetSRV(RHI_SRV_Index::DofCocMap)
	};
	ID3D11SamplerState* LinearClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 2, Srvs);
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, &LinearClampSampler);

	RHIDevice->DrawFullScreenQuad();

	Swap.Commit();
}

void FDepthOfFieldPass::ExecuteUpscalePass(FSceneView* View, D3D11RHI* RHIDevice)
{
}

void FDepthOfFieldPass::ExecuteTemporalBlendPass(FSceneView* View, D3D11RHI* RHIDevice)
{
}

void FDepthOfFieldPass::ExecuteCompositePass(const FPostProcessModifier& M, FSceneView* View, D3D11RHI* RHIDevice)
{
	// Composite 패스: 원본 씬과 블러 결과를 CoC 맵을 기준으로 블렌딩하여 최종 결과를 SceneColorTarget에 저장
	UDepthOfFieldComponent* DofComponent = Cast<UDepthOfFieldComponent>(M.SourceObject);
	if (!DofComponent) { return; }

	// 이전 단계의 RTV와 SRV 상태를 완전히 끊어줍니다.
	ID3D11RenderTargetView* nullRTV = nullptr;
	RHIDevice->GetDeviceContext()->OMSetRenderTargets(1, &nullRTV, nullptr);

	ID3D11ShaderResourceView* nullSRVs[3] = { nullptr, nullptr, nullptr };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 3, nullSRVs);

	// 1) 스왑 + SRV 언바인드 관리
	FSwapGuard Swap(RHIDevice, /*FirstSlot*/0, /*NumSlotsToUnbind*/3);

	// 2) SceneColorTarget으로 렌더링 (최종 출력)
	RHIDevice->OMSetRenderTargets(ERTVMode::SceneColorTargetWithoutDepth);

	// Depth State: Depth Test/Write 모두 OFF
	RHIDevice->OMSetDepthStencilState(EComparisonFunc::Always);
	RHIDevice->OMSetBlendState(false);

	// 3) 셰이더 로드
	UShader* FullScreenTriangleVS = UResourceManager::GetInstance().Load<UShader>("Shaders/Utility/FullScreenTriangle_VS.hlsl");
	UShader* CompositePS = UResourceManager::GetInstance().Load<UShader>("Shaders/PostProcess/DoF_Composite_PS.hlsl");
	if (!FullScreenTriangleVS || !FullScreenTriangleVS->GetVertexShader() || !CompositePS || !CompositePS->GetPixelShader())
	{
		UE_LOG("DoF Composite 셰이더 없음!\n");
		return;
	}

	RHIDevice->PrepareShader(FullScreenTriangleVS, CompositePS);

	// 4) SRV/Sampler (원본 씬 + 블러 결과 + CoC 맵)
	ID3D11ShaderResourceView* SceneSRV = RHIDevice->GetSRV(RHI_SRV_Index::SceneColorSource);
	ID3D11ShaderResourceView* BlurMapSRV = RHIDevice->GetSRV(RHI_SRV_Index::DofBlurMap);
	ID3D11ShaderResourceView* CoCMapSRV = RHIDevice->GetSRV(RHI_SRV_Index::DofCocMap);
	ID3D11SamplerState* LinearClampSampler = RHIDevice->GetSamplerState(RHI_Sampler_Index::LinearClamp);

	if (!SceneSRV || !BlurMapSRV || !CoCMapSRV || !LinearClampSampler)
	{
		UE_LOG("DoF Composite: Required SRVs or Samplers are null!\n");
		return;
	}

	ID3D11ShaderResourceView* Srvs[3] = { SceneSRV, BlurMapSRV, CoCMapSRV };
	RHIDevice->GetDeviceContext()->PSSetShaderResources(0, 3, Srvs);

	ID3D11SamplerState* Smps[1] = { LinearClampSampler };
	RHIDevice->GetDeviceContext()->PSSetSamplers(0, 1, Smps);

	// 5) 상수 버퍼 업데이트
	ECameraProjectionMode ProjectionMode = View->ProjectionMode;
	RHIDevice->SetAndUpdateConstantBuffer(PostProcessBufferType(View->NearClip, View->FarClip, ProjectionMode == ECameraProjectionMode::Orthographic));

	// DoF 설정 가져오기
	FDepthOfFieldSettings DofSettings = DofComponent->GetDepthOfFieldSettings();
	FDepthOfFieldBufferType DofBuffer;
	DofBuffer.FocalDistance = DofSettings.FocalDistance;
	DofBuffer.FocalLength = DofSettings.FocalLength;
	DofBuffer.FNumber = DofSettings.Aperture;
	DofBuffer.MaxCoc = DofSettings.MaxCocRadius;
	DofBuffer.NearTransitionRange = DofSettings.NearTransitionRange;
	DofBuffer.FarTransitionRange = DofSettings.FarTransitionRange;
	DofBuffer.NearBlurScale = DofSettings.NearBlurScale;
	DofBuffer.FarBlurScale = DofSettings.FarBlurScale;
	DofBuffer.BlurSampleCount = DofSettings.BlurSampleCount;
	DofBuffer.BokehRotation = DofSettings.BokehRotationRadians;
	DofBuffer.Weight = M.Weight;
	DofBuffer._Pad = 0.0f;

	RHIDevice->SetAndUpdateConstantBuffer(DofBuffer);

	// 6) Draw
	RHIDevice->DrawFullScreenQuad();

	// 7) 확정
	Swap.Commit();
}
