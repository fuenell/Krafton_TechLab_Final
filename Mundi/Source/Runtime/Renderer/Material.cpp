#include "pch.h"
#include "Material.h"
#include "Shader.h"
#include "Texture.h"
#include "ResourceManager.h"

//IMPLEMENT_CLASS(UMaterial)

UMaterial::UMaterial()
{
	// 배열 크기를 미리 할당 (Enum 값 사용)
	ResolvedTextures.resize(static_cast<size_t>(EMaterialTextureSlot::Max));
}

// 해당 경로의 셰이더 또는 텍스쳐를 로드해서 머티리얼로 생성 후 반환한다
void UMaterial::Load(const FString& InFilePath, ID3D11Device* InDevice)
{
	MaterialInfo.MaterialName = InFilePath;

	// 기본 쉐이더 로드 (LayoutType에 따라)
	// dds 의 경우
	if (InFilePath.find(".dds") != std::string::npos)
	{
		FString shaderName = UResourceManager::GetInstance().GetProperShader(InFilePath);

		Shader = UResourceManager::GetInstance().Load<UShader>(shaderName);
		UResourceManager::GetInstance().Load<UTexture>(InFilePath);
		MaterialInfo.DiffuseTextureFileName = InFilePath;
	} // hlsl 의 경우
	else if (InFilePath.find(".hlsl") != std::string::npos)
	{
		Shader = UResourceManager::GetInstance().Load<UShader>(InFilePath);
	}
	else
	{
		throw std::runtime_error(".dds나 .hlsl만 입력해주세요. 현재 입력 파일명 : " + InFilePath);
	}
}

UMaterial* UMaterial::CreateWithShaderOverride(UMaterialInterface* SourceMaterial, UShader* OverrideShader)
{
	if (!SourceMaterial || !OverrideShader)
	{
		return nullptr;
	}

	UMaterial* NewMaterial = NewObject<UMaterial>();

	// ===== 깊은 복사 (Deep Copy) =====
	// MaterialInfo: 값 타입이므로 대입 연산자로 깊은 복사
	// (색상, 텍스처 경로 문자열 등 모든 값 데이터 복사)
	NewMaterial->MaterialInfo = SourceMaterial->GetMaterialInfo();

	// ===== 얕은 복사 (Shallow Copy) =====
	// ResolvedTextures: 텍스처 리소스는 ResourceManager가 소유하므로 포인터만 공유
	for (uint8 i = 0; i < static_cast<uint8>(EMaterialTextureSlot::Max); ++i)
	{
		EMaterialTextureSlot Slot = static_cast<EMaterialTextureSlot>(i);
		NewMaterial->ResolvedTextures[i] = SourceMaterial->GetTexture(Slot);
	}

	// ===== 교체 (Replace) =====
	// Shader: 새 인스턴싱 셰이더로 교체 (원본 셰이더와 다름)
	NewMaterial->Shader = OverrideShader;

	// ===== 복사 안 함 (Skip) =====
	// ShaderMacros: 새 셰이더용 매크로와 호환되지 않을 수 있으므로 빈 상태 유지

	return NewMaterial;
}

void UMaterial::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	Super::Serialize(bInIsLoading, InOutHandle); // 부모 Serialize 호출

	if (bInIsLoading)
	{
		// UStaticMeshComponent가 이미 UMaterial 객체를 생성한 후
		// 이 함수를 호출했다고 가정합니다.
	}
	else // 저장
	{
		InOutHandle["AssetPath"] = GetFilePath();
	}
}

void UMaterial::SetShader(UShader* InShaderResource)
{
	Shader = InShaderResource;
}

void UMaterial::SetShaderByName(const FString& InShaderName)
{
	SetShader(UResourceManager::GetInstance().Load<UShader>(InShaderName));
}

UShader* UMaterial::GetShader()
{
	return Shader;
}

const TArray<FShaderMacro> UMaterial::GetShaderMacros() const
{
	return ShaderMacros;
}

void UMaterial::SetShaderMacros(const TArray<FShaderMacro>& InShaderMacro)
{
	if (Shader)
	{
		UResourceManager::GetInstance().Load<UShader>(Shader->GetFilePath(), InShaderMacro);
	}

	ShaderMacros = InShaderMacro;
}

UTexture* UMaterial::GetTexture(EMaterialTextureSlot Slot) const
{
	size_t Index = static_cast<size_t>(Slot);
	// std::vector의 유효 인덱스 검사
	if (Index < ResolvedTextures.size())
	{
		return ResolvedTextures[Index];
	}
	return nullptr;
}

const FMaterialInfo& UMaterial::GetMaterialInfo() const
{
	return MaterialInfo;
}

bool UMaterial::HasTexture(EMaterialTextureSlot Slot) const
{
	switch (Slot)
	{
	case EMaterialTextureSlot::Diffuse:
		return !MaterialInfo.DiffuseTextureFileName.empty();
	case EMaterialTextureSlot::Normal:
		return !MaterialInfo.NormalTextureFileName.empty();
	default:
		return false;
	}
}

void UMaterial::ResolveTextures()
{
	auto& RM = UResourceManager::GetInstance();
	size_t MaxSlots = static_cast<size_t>(EMaterialTextureSlot::Max);

	// 배열 크기가 Enum 크기와 맞는지 확인 (안전장치)
	if (ResolvedTextures.size() != MaxSlots)
	{
		ResolvedTextures.resize(MaxSlots);
	}

	// 각 슬롯에 해당하는 텍스처 경로로 UTexture* 찾아서 배열에 저장
	if (!MaterialInfo.DiffuseTextureFileName.empty())
		ResolvedTextures[static_cast<int32>(EMaterialTextureSlot::Diffuse)] = RM.Load<UTexture>(MaterialInfo.DiffuseTextureFileName, true);
	else
		ResolvedTextures[static_cast<int32>(EMaterialTextureSlot::Diffuse)] = nullptr; // 또는 기본 텍스처

	if (!MaterialInfo.NormalTextureFileName.empty())
		ResolvedTextures[static_cast<int32>(EMaterialTextureSlot::Normal)] = RM.Load<UTexture>(MaterialInfo.NormalTextureFileName, false);
	else
		ResolvedTextures[static_cast<int32>(EMaterialTextureSlot::Normal)] = nullptr; // 또는 기본 노멀 텍스처
}

void UMaterial::SetMaterialInfo(const FMaterialInfo& InMaterialInfo)
{
	MaterialInfo = InMaterialInfo;
	ResolveTextures();
}
