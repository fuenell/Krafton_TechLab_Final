#pragma once
#include "MaterialInterface.h"
#include "UMaterial.generated.h"

// 머티리얼 에셋 인스턴스
UCLASS()
class UMaterial : public UMaterialInterface
{
	GENERATED_REFLECTION_BODY()
public:
	UMaterial();
	void Load(const FString& InFilePath, ID3D11Device* InDevice);

	// 원본 Material의 속성(텍스처, 색상 등)을 복사하고 셰이더만 교체한 새 Material 생성
	// 파티클 메시처럼 인스턴싱 전용 셰이더가 필요한 경우 사용
	static UMaterial* CreateWithShaderOverride(UMaterialInterface* SourceMaterial, UShader* OverrideShader);

protected:
	~UMaterial() override = default;

public:
	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void SetShader(UShader* InShaderResource);
	void SetShaderByName(const FString& InShaderName);
	UShader* GetShader() override;
	UTexture* GetTexture(EMaterialTextureSlot Slot) const override;
	bool HasTexture(EMaterialTextureSlot Slot) const override;
	const FMaterialInfo& GetMaterialInfo() const override;

	// MaterialInfo의 텍스처 경로들을 기반으로 ResolvedTextures 배열을 채웁니다.
	void ResolveTextures();

	void SetMaterialInfo(const FMaterialInfo& InMaterialInfo);

	void SetMaterialName(FString& InMaterialName) { MaterialInfo.MaterialName = InMaterialName; }

	const TArray<FShaderMacro> GetShaderMacros() const override;
	void SetShaderMacros(const TArray<FShaderMacro>& InShaderMacro);

protected:
	// 이 머티리얼이 사용할 셰이더 프로그램 (예: UberLit.hlsl)
	UShader* Shader = nullptr;
	TArray<FShaderMacro> ShaderMacros;

	FMaterialInfo MaterialInfo;
	// MaterialInfo 이름 기반으로 찾은 (Textures[0] = Diffuse, Textures[1] = Normal)
	TArray<UTexture*> ResolvedTextures;
};
