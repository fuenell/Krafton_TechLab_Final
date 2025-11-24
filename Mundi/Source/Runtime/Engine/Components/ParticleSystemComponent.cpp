#include "pch.h"
#include "ParticleSystemComponent.h"
#include "SceneView.h"
#include "MeshBatchElement.h"
#include "Material.h"
#include "Shader.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Modules/ParticleModuleSpawn.h"
#include "Modules/ParticleModuleLifetime.h"
#include "Modules/ParticleModuleVelocity.h"

UParticleSystemComponent::UParticleSystemComponent()
{
	bAutoActivate = true;
	bCanEverTick = true;   // Tick 활성화
	bTickInEditor = true;  // 에디터에서도 파티클 미리보기 가능
}

UParticleSystemComponent::~UParticleSystemComponent()
{
	// OnUnregister에서 이미 정리되었으므로, 안전 체크만 수행
	// (만약 OnUnregister가 호출되지 않은 경우를 대비)
	if (EmitterInstances.Num() > 0)
	{
		ClearEmitterInstances();
	}

	if (EmitterRenderData.Num() > 0)
	{
		for (FDynamicEmitterDataBase* RenderData : EmitterRenderData)
		{
			if (RenderData)
			{
				delete RenderData;
			}
		}
		EmitterRenderData.Empty();
	}

	// 버퍼 해제
	if (ParticleVertexBuffer)
	{
		ParticleVertexBuffer->Release();
		ParticleVertexBuffer = nullptr;
	}
	if (ParticleIndexBuffer)
	{
		ParticleIndexBuffer->Release();
		ParticleIndexBuffer = nullptr;
	}
}

void UParticleSystemComponent::OnRegister(UWorld* InWorld)
{
	Super::OnRegister(InWorld);

	// DEBUG: OnRegister 호출 확인 (추후 제거)
	UE_LOG("[ParticleSystemComponent::OnRegister] Called! World=%p, EmitterCount=%d",
		InWorld, EmitterInstances.Num());

	// 이미 초기화되어 있으면 스킵 (OnRegister는 여러 번 호출될 수 있음)
	if (EmitterInstances.Num() > 0)
	{
		return;
	}

	// Template이 없으면 디버그용 기본 파티클 시스템 생성
	if (!Template)
	{
		CreateDebugParticleSystem();
	}

	// 에디터에서도 파티클 미리보기를 위해 자동 활성화
	if (bAutoActivate)
	{
		ActivateSystem();
		UE_LOG("[ParticleSystemComponent::OnRegister] ActivateSystem called, EmitterCount=%d",
			EmitterInstances.Num());
	}
}

void UParticleSystemComponent::CreateDebugParticleSystem()
{
	// 디버그/테스트용 기본 파티클 시스템 생성
	// Editor 통합 완료 후에는 Editor에서 설정한 Template 사용
	Template = NewObject<UParticleSystem>();

	// 이미터 생성
	UParticleEmitter* Emitter = NewObject<UParticleEmitter>();

	// LOD 레벨 생성
	UParticleLODLevel* LODLevel = NewObject<UParticleLODLevel>();
	LODLevel->bEnabled = true;

	// 필수 모듈 생성
	LODLevel->RequiredModule = NewObject<UParticleModuleRequired>();

	// 스폰 모듈 생성
	UParticleModuleSpawn* SpawnModule = NewObject<UParticleModuleSpawn>();
	SpawnModule->SpawnRate = 20.0f;  // 초당 20개 파티클
	SpawnModule->BurstCount = 10;    // 시작 시 10개 버스트
	LODLevel->SpawnModule = SpawnModule;  // SpawnModule 필드에 할당 (Tick에서 직접 접근)
	LODLevel->Modules.Add(SpawnModule);

	// 라이프타임 모듈 생성 (파티클 수명 설정)
	UParticleModuleLifetime* LifetimeModule = NewObject<UParticleModuleLifetime>();
	LifetimeModule->MinLifetime = 2.0f;  // 최소 2초
	LifetimeModule->MaxLifetime = 3.0f;  // 최대 3초
	LODLevel->Modules.Add(LifetimeModule);

	// 속도 모듈 생성 (테스트용: 위쪽으로 퍼지는 랜덤 속도)
	UParticleModuleVelocity* VelocityModule = NewObject<UParticleModuleVelocity>();
	VelocityModule->StartVelocity = FVector(0.0f, 0.0f, 100.0f);  // 기본 위쪽 속도
	VelocityModule->StartVelocityRange = FVector(50.0f, 50.0f, 50.0f);  // XYZ 랜덤 범위
	LODLevel->Modules.Add(VelocityModule);

	// 모듈 캐싱
	LODLevel->CacheModuleInfo();

	// LOD 레벨을 이미터에 추가
	Emitter->LODLevels.Add(LODLevel);
	Emitter->CacheEmitterModuleInfo();

	// 이미터를 시스템에 추가
	Template->Emitters.Add(Emitter);
}

void UParticleSystemComponent::OnUnregister()
{
	// 이미터 인스턴스 정리
	DeactivateSystem();

	// 렌더 데이터 정리
	for (FDynamicEmitterDataBase* RenderData : EmitterRenderData)
	{
		if (RenderData)
		{
			delete RenderData;
		}
	}
	EmitterRenderData.Empty();

	Super::OnUnregister();
}

void UParticleSystemComponent::TickComponent(float DeltaTime)
{
	USceneComponent::TickComponent(DeltaTime);

	// 모든 이미터 인스턴스 틱
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance)
		{
			Instance->Tick(DeltaTime, false);
		}
	}

	// 렌더 데이터 업데이트
	UpdateRenderData();
}

void UParticleSystemComponent::ActivateSystem()
{
	if (Template)
	{
		InitializeEmitterInstances();
	}
}

void UParticleSystemComponent::DeactivateSystem()
{
	ClearEmitterInstances();
}

void UParticleSystemComponent::ResetParticles()
{
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance)
		{
			Instance->KillAllParticles();
		}
	}
}

void UParticleSystemComponent::SetTemplate(UParticleSystem* NewTemplate)
{
	if (Template != NewTemplate)
	{
		Template = NewTemplate;

		// 활성 상태면 재초기화
		if (EmitterInstances.Num() > 0)
		{
			ClearEmitterInstances();
			InitializeEmitterInstances();
		}
	}
}

void UParticleSystemComponent::InitializeEmitterInstances()
{
	ClearEmitterInstances();

	if (!Template)
	{
		return;
	}

	// 이미터 인스턴스 생성
	for (UParticleEmitter* Emitter : Template->Emitters)
	{
		if (Emitter)
		{
			FParticleEmitterInstance* Instance = new FParticleEmitterInstance();
			Instance->Init(this, Emitter);
			EmitterInstances.Add(Instance);
		}
	}
}

void UParticleSystemComponent::ClearEmitterInstances()
{
	for (FParticleEmitterInstance* Instance : EmitterInstances)
	{
		if (Instance)
		{
			delete Instance;
		}
	}
	EmitterInstances.Empty();
}

void UParticleSystemComponent::UpdateRenderData()
{
	// 기존 렌더 데이터 제거
	for (FDynamicEmitterDataBase* RenderData : EmitterRenderData)
	{
		if (RenderData)
		{
			delete RenderData;
		}
	}
	EmitterRenderData.Empty();

	// 각 이미터 인스턴스에서 GetDynamicData() 호출 (캡슐화된 패턴)
	for (int32 i = 0; i < EmitterInstances.Num(); i++)
	{
		FParticleEmitterInstance* Instance = EmitterInstances[i];
		if (!Instance)
		{
			continue;
		}

		// EmitterInstance가 자신의 렌더 데이터를 생성
		FDynamicEmitterDataBase* DynamicData = Instance->GetDynamicData(false);
		if (DynamicData)
		{
			DynamicData->EmitterIndex = i;
			EmitterRenderData.Add(DynamicData);
		}
	}
}

// 언리얼 엔진 호환: 인스턴스 파라미터 시스템 구현
void UParticleSystemComponent::SetFloatParameter(const FString& ParameterName, float Value)
{
	// 기존 파라미터 찾기
	for (FParticleParameter& Param : InstanceParameters)
	{
		if (Param.Name == ParameterName)
		{
			Param.FloatValue = Value;
			return;
		}
	}

	// 없으면 새로 추가
	FParticleParameter NewParam(ParameterName);
	NewParam.FloatValue = Value;
	InstanceParameters.Add(NewParam);
}

void UParticleSystemComponent::SetVectorParameter(const FString& ParameterName, const FVector& Value)
{
	// 기존 파라미터 찾기
	for (FParticleParameter& Param : InstanceParameters)
	{
		if (Param.Name == ParameterName)
		{
			Param.VectorValue = Value;
			return;
		}
	}

	// 없으면 새로 추가
	FParticleParameter NewParam(ParameterName);
	NewParam.VectorValue = Value;
	InstanceParameters.Add(NewParam);
}

void UParticleSystemComponent::SetColorParameter(const FString& ParameterName, const FLinearColor& Value)
{
	// 기존 파라미터 찾기
	for (FParticleParameter& Param : InstanceParameters)
	{
		if (Param.Name == ParameterName)
		{
			Param.ColorValue = Value;
			return;
		}
	}

	// 없으면 새로 추가
	FParticleParameter NewParam(ParameterName);
	NewParam.ColorValue = Value;
	InstanceParameters.Add(NewParam);
}

float UParticleSystemComponent::GetFloatParameter(const FString& ParameterName, float DefaultValue) const
{
	for (const FParticleParameter& Param : InstanceParameters)
	{
		if (Param.Name == ParameterName)
		{
			return Param.FloatValue;
		}
	}
	return DefaultValue;
}

FVector UParticleSystemComponent::GetVectorParameter(const FString& ParameterName, const FVector& DefaultValue) const
{
	for (const FParticleParameter& Param : InstanceParameters)
	{
		if (Param.Name == ParameterName)
		{
			return Param.VectorValue;
		}
	}
	return DefaultValue;
}

FLinearColor UParticleSystemComponent::GetColorParameter(const FString& ParameterName, const FLinearColor& DefaultValue) const
{
	for (const FParticleParameter& Param : InstanceParameters)
	{
		if (Param.Name == ParameterName)
		{
			return Param.ColorValue;
		}
	}
	return DefaultValue;
}

void UParticleSystemComponent::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	USceneComponent::Serialize(bInIsLoading, InOutHandle);

	// UPROPERTY 속성은 리플렉션 시스템에 의해 자동으로 직렬화됨

	if (!bInIsLoading && bAutoActivate)
	{
		// 로딩 후, 자동 활성화가 켜져 있으면 시스템 활성화
		ActivateSystem();
	}
}

void UParticleSystemComponent::CollectMeshBatches(TArray<FMeshBatchElement>& OutMeshBatchElements, const FSceneView* View)
{
	// 1. 유효성 검사
	if (!IsVisible() || EmitterRenderData.Num() == 0)
	{
		return;
	}

	// 2. 전체 파티클 수 계산 및 정렬
	int32 TotalParticles = 0;
	for (FDynamicEmitterDataBase* EmitterData : EmitterRenderData)
	{
		if (!EmitterData)
			continue;

		const FDynamicEmitterReplayDataBase& Source = EmitterData->GetSource();
		TotalParticles += Source.ActiveParticleCount;

		// 스프라이트 이미터에 대해 정렬 수행
		if (Source.eEmitterType == EDynamicEmitterType::Sprite)
		{
			auto* SpriteData = static_cast<FDynamicSpriteEmitterDataBase*>(EmitterData);
			// View 원점을 기준으로 파티클 정렬 (투명 렌더링을 위해 먼 것부터)
			FVector ViewOrigin = View ? View->ViewLocation : FVector(0.0f, 0.0f, 0.0f);
			SpriteData->SortSpriteParticles(Source.SortMode, ViewOrigin);
		}
	}

	if (TotalParticles == 0)
	{
		return;
	}

	// 3. 버퍼 크기 계산 (파티클당 4 버텍스, 6 인덱스 - 쿼드)
	uint32 RequiredVertexCount = TotalParticles * 4;
	uint32 RequiredIndexCount = TotalParticles * 6;

	// 4. 버퍼 생성/리사이즈
	EnsureBufferSize(RequiredVertexCount, RequiredIndexCount);

	// 5. 버텍스 데이터 채우기
	FillVertexBuffer(View);

	// 6. FMeshBatchElement 생성
	CreateMeshBatch(OutMeshBatchElements, RequiredIndexCount);
}

// 추가할 함수의 구현
// - EnsureBufferSize: 필요한 정점/인덱스 수를 확인하고 내부 할당 상태를 갱신합니다.
// - FillVertexBuffer: 현재 뷰를 기준으로 정점 버퍼를 채우는 자리표시자(간단한 안전 구현).
// - CreateMeshBatch: 계산된 정점/인덱스 카운트로 FMeshBatchElement을 생성하여 출력 배열에 추가합니다.

void UParticleSystemComponent::EnsureBufferSize(uint32 RequiredVertexCount, uint32 RequiredIndexCount)
{
	ID3D11Device* Device = GEngine.GetRHIDevice()->GetDevice();

	// Vertex Buffer 생성/리사이즈
	if (RequiredVertexCount > AllocatedVertexCount)
	{
		if (ParticleVertexBuffer)
		{
			ParticleVertexBuffer->Release();
			ParticleVertexBuffer = nullptr;
		}

		// 성장 전략: 2배 또는 최소 128개
		uint32 NewCount = FMath::Max(RequiredVertexCount * 2, 128u);

		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = NewCount * sizeof(FParticleSpriteVertex);
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = Device->CreateBuffer(&Desc, nullptr, &ParticleVertexBuffer);
		if (SUCCEEDED(hr))
		{
			AllocatedVertexCount = NewCount;
		}
	}

	// Index Buffer 생성/리사이즈
	if (RequiredIndexCount > AllocatedIndexCount)
	{
		if (ParticleIndexBuffer)
		{
			ParticleIndexBuffer->Release();
			ParticleIndexBuffer = nullptr;
		}

		uint32 NewCount = FMath::Max(RequiredIndexCount * 2, 256u);

		D3D11_BUFFER_DESC Desc = {};
		Desc.ByteWidth = NewCount * sizeof(uint32);
		Desc.Usage = D3D11_USAGE_DYNAMIC;
		Desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		HRESULT hr = Device->CreateBuffer(&Desc, nullptr, &ParticleIndexBuffer);
		if (SUCCEEDED(hr))
		{
			AllocatedIndexCount = NewCount;
		}
	}
}

void UParticleSystemComponent::FillVertexBuffer(const FSceneView* View)
{
	if (!ParticleVertexBuffer || !ParticleIndexBuffer || !View)
	{
		return;
	}

	// DEBUG: 위치 정보 확인 (추후 제거)
	static int DebugCounter = 0;
	if (DebugCounter++ % 60 == 0)  // 1초에 한 번만 출력
	{
		FVector WorldLoc = GetWorldLocation();
		FVector RelLoc = GetRelativeLocation();
		USceneComponent* Parent = GetAttachParent();
		UE_LOG("[FillVertexBuffer] WorldLoc=(%.1f, %.1f, %.1f), RelLoc=(%.1f, %.1f, %.1f), AttachParent=%p",
			WorldLoc.X, WorldLoc.Y, WorldLoc.Z,
			RelLoc.X, RelLoc.Y, RelLoc.Z,
			Parent);
	}

	ID3D11DeviceContext* Context = GEngine.GetRHIDevice()->GetDeviceContext();

	// Vertex Buffer Map
	D3D11_MAPPED_SUBRESOURCE MappedVertex;
	HRESULT hr = Context->Map(ParticleVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedVertex);
	if (FAILED(hr))
	{
		return;
	}

	// Index Buffer Map
	D3D11_MAPPED_SUBRESOURCE MappedIndex;
	hr = Context->Map(ParticleIndexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedIndex);
	if (FAILED(hr))
	{
		Context->Unmap(ParticleVertexBuffer, 0);
		return;
	}

	FParticleSpriteVertex* Vertices = static_cast<FParticleSpriteVertex*>(MappedVertex.pData);
	uint32* Indices = static_cast<uint32*>(MappedIndex.pData);

	uint32 VertexOffset = 0;
	uint32 IndexOffset = 0;

	// 쿼드 UV 좌표
	const FVector2D QuadUVs[4] = {
		FVector2D(0.0f, 1.0f),  // 왼쪽 하단
		FVector2D(0.0f, 0.0f),  // 왼쪽 상단
		FVector2D(1.0f, 0.0f),  // 오른쪽 상단
		FVector2D(1.0f, 1.0f)   // 오른쪽 하단
	};

	// 모든 이미터의 파티클 처리
	for (int32 EmitterIdx = 0; EmitterIdx < EmitterRenderData.Num(); EmitterIdx++)
	{
		FDynamicEmitterDataBase* EmitterData = EmitterRenderData[EmitterIdx];
		if (!EmitterData)
		{
			continue;
		}

		const FDynamicEmitterReplayDataBase& Source = EmitterData->GetSource();
		const int32 ParticleCount = Source.ActiveParticleCount;
		const int32 ParticleStride = Source.ParticleStride;
		const uint8* ParticleData = Source.DataContainer.ParticleData;
		const uint16* ParticleIndices = Source.DataContainer.ParticleIndices;

		if (!ParticleData || ParticleCount == 0)
		{
			continue;
		}

		// 각 파티클에 대해 쿼드 생성
		for (int32 ParticleIdx = 0; ParticleIdx < ParticleCount; ParticleIdx++)
		{
			// 파티클 인덱스로 실제 파티클 데이터 접근
			const int32 ParticleIndex = ParticleIndices ? ParticleIndices[ParticleIdx] : ParticleIdx;
			const FBaseParticle* Particle = reinterpret_cast<const FBaseParticle*>(
				ParticleData + ParticleIndex * ParticleStride
			);

			// 컴포넌트 월드 트랜스폼 적용
			FVector ParticleWorldPos = GetWorldLocation() + Particle->Location * GetRelativeScale();

			// DEBUG: 첫 번째 파티클의 위치 정보 (추후 제거)
			if (ParticleIdx == 0 && DebugCounter % 60 == 1)
			{
				UE_LOG("[Particle] LocalLoc=(%.1f, %.1f, %.1f), WorldPos=(%.1f, %.1f, %.1f)",
					Particle->Location.X, Particle->Location.Y, Particle->Location.Z,
					ParticleWorldPos.X, ParticleWorldPos.Y, ParticleWorldPos.Z);
			}
			FVector2D ParticleSize = FVector2D(Particle->Size.X, Particle->Size.Y) * GetRelativeScale().X;

				// 4개 버텍스 생성 (빌보드 정렬 및 Z회전은 GPU에서 수행)
			for (int32 v = 0; v < 4; v++)
			{
				FParticleSpriteVertex& Vertex = Vertices[VertexOffset + v];
				Vertex.WorldPosition = ParticleWorldPos;  // 모든 버텍스가 동일한 중심 위치
				Vertex.Rotation = Particle->Rotation;
				Vertex.UV = QuadUVs[v];
				Vertex.Size = ParticleSize;
				Vertex.Color = Particle->Color;
				Vertex.RelativeTime = Particle->RelativeTime;
			}

			TArray<int> Order = { 0, 1, 2, 0, 2, 3 };

			for (int TriIdx = 0; TriIdx < Order.Num(); ++TriIdx)
				Indices[IndexOffset + TriIdx] = VertexOffset + Order[TriIdx];

			VertexOffset += 4;
			IndexOffset += 6;
		}
	}

	Context->Unmap(ParticleIndexBuffer, 0);
	Context->Unmap(ParticleVertexBuffer, 0);
}

void UParticleSystemComponent::CreateMeshBatch(TArray<FMeshBatchElement>& OutMeshBatchElements, uint32 IndexCount)
{
	if (!ParticleVertexBuffer || !ParticleIndexBuffer || IndexCount == 0)
	{
		return;
	}

	// 첫 번째 스프라이트 이미터에서 Material과 VertexStride 가져오기
	UMaterialInterface* Material = nullptr;
	int32 VertexStride = sizeof(FParticleSpriteVertex);  // 기본값

	for (FDynamicEmitterDataBase* EmitterData : EmitterRenderData)
	{
		if (!EmitterData)
			continue;

		const FDynamicEmitterReplayDataBase& Source = EmitterData->GetSource();
		if (Source.eEmitterType == EDynamicEmitterType::Sprite)
		{
			// 스프라이트 이미터에서 Material 가져오기
			const auto& SpriteSource = static_cast<const FDynamicSpriteEmitterReplayDataBase&>(Source);
			if (SpriteSource.MaterialInterface)
			{
				Material = SpriteSource.MaterialInterface;
			}

			// GetDynamicVertexStride() 사용
			auto* SpriteData = static_cast<FDynamicSpriteEmitterDataBase*>(EmitterData);
			VertexStride = SpriteData->GetDynamicVertexStride();
			break;  // 첫 번째 유효한 스프라이트 이미터 사용
		}
	}

	// Material이 없으면 기본 파티클 셰이더 사용 (폴백)
	if (!Material)
	{
		Material = UResourceManager::GetInstance().Load<UMaterial>("Shaders/Particle/ParticleSprite.hlsl");
	}

	if (!Material || !Material->GetShader())
	{
		return;
	}

	UShader* Shader = Material->GetShader();
	FShaderVariant* ShaderVariant = Shader->GetOrCompileShaderVariant(Material->GetShaderMacros());

	// FMeshBatchElement 생성
	FMeshBatchElement BatchElement;

	BatchElement.VertexShader = ShaderVariant->VertexShader;
	BatchElement.PixelShader = ShaderVariant->PixelShader;
	BatchElement.InputLayout = ShaderVariant->InputLayout;
	BatchElement.Material = Material;
	BatchElement.VertexBuffer = ParticleVertexBuffer;
	BatchElement.IndexBuffer = ParticleIndexBuffer;
	BatchElement.VertexStride = VertexStride;

	BatchElement.IndexCount = IndexCount;
	BatchElement.StartIndex = 0;
	BatchElement.BaseVertexIndex = 0;

	// 월드 행렬은 항등 행렬 (버텍스가 이미 월드 좌표)
	BatchElement.WorldMatrix = FMatrix::Identity();
	BatchElement.ObjectID = InternalIndex;
	BatchElement.PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// Material에서 텍스처 가져오기 (Diffuse 슬롯)
	UTexture* ParticleTexture = Material->GetTexture(EMaterialTextureSlot::Diffuse);
	if (!ParticleTexture)
	{
		// 폴백: 기본 텍스처 로드
		ParticleTexture = UResourceManager::GetInstance().Load<UTexture>(GDataDir + "/cube_texture.png");
	}

	if (ParticleTexture && ParticleTexture->GetShaderResourceView())
	{
		BatchElement.InstanceShaderResourceView = ParticleTexture->GetShaderResourceView();
	}

	OutMeshBatchElements.Add(BatchElement);
}
