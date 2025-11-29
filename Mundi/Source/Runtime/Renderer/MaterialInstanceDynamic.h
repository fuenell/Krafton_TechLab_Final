#pragma once
#include "MaterialInterface.h"
#include "UMaterialInstanceDynamic.generated.h"

// 동적 머티리얼 인스턴스
UCLASS()
class UMaterialInstanceDynamic : public UMaterialInterface
{
	GENERATED_REFLECTION_BODY()
public:
	UMaterialInstanceDynamic();

	// 부모 머티리얼을 기반으로 인스턴스를 생성하는 정적 함수
	static UMaterialInstanceDynamic* Create(UMaterialInterface* InParentMaterial);

	void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;

	void CopyParametersFrom(const UMaterialInstanceDynamic* Other);

	// UMaterialInterface의 순수 가상 함수들을 구현합니다.
	UShader* GetShader() override;
	UTexture* GetTexture(EMaterialTextureSlot Slot) const override;
	bool HasTexture(EMaterialTextureSlot Slot) const override;
	const FMaterialInfo& GetMaterialInfo() const override;
	UMaterialInterface* GetParentMaterial() const { return ParentMaterial; }

	const TArray<FShaderMacro> GetShaderMacros() const override;	// 이 인스턴스에 덮어쓴 매크로가 없다면 부모의 매크로를, 있다면 덮어쓴 매크로를 반환합니다.

	const TMap<EMaterialTextureSlot, UTexture*>& GetOverriddenTextures() const { return OverriddenTextures; }	// 덮어쓴 텍스처 맵 반환 (저장 시 사용)
	void SetTextureParameterValue(EMaterialTextureSlot Slot, UTexture* Value);	// 텍스처 파라미터 값을 런타임에 변경하는 함수 (실시간 수정 시 사용)
	void SetOverriddenTextureParameters(const TMap<EMaterialTextureSlot, UTexture*>& InTextures);	// 덮어쓴 텍스처 맵 설정 (로드 시 사용)

	const TMap<FString, float>& GetOverriddenScalarParameters() const { return OverriddenScalarParameters; }	// 덮어쓴 스칼라 맵 반환 (저장 시 사용)
	void SetScalarParameterValue(const FString& ParameterName, float Value);	// 스칼라 파라미터 값을 런타임에 변경하는 함수 (실시간 수정 시 사용)
	void SetOverriddenScalarParameters(const TMap<FString, float>& InScalars);	// 덮어쓴 스칼라 맵 설정 (로드 시 사용)

	const TMap<FString, FLinearColor>& GetOverriddenVectorParameters() const { return OverriddenColorParameters; }	// 덮어쓴 벡터 맵 반환 (저장 시 사용)
	void SetColorParameterValue(const FString& ParameterName, const FLinearColor& Value);	// 벡터 파라미터 값을 런타임에 변경하는 함수 (실시간 수정 시 사용)
	void SetOverriddenVectorParameters(const TMap<FString, FLinearColor>& InVectors);	// 덮어쓴 벡터 맵 설정 (로드 시 사용)

protected:
	// 생성자에서 부모 머티리얼의 포인터를 저장합니다.
	UMaterialInstanceDynamic(UMaterialInterface* InParentMaterial);

private:
	UMaterialInterface* ParentMaterial{};

	// 이 인스턴스에서 덮어쓴 값들만 저장합니다.
	TMap<EMaterialTextureSlot, UTexture*> OverriddenTextures;
	TMap<FString, float> OverriddenScalarParameters;
	TMap<FString, FLinearColor> OverriddenColorParameters;

	// GetMaterialInfo()가 호출될 때마다 부모의 정보를 복사하고
	// 아래 값들로 덮어쓴 뒤 반환하기 위한 캐시 데이터입니다.
	mutable FMaterialInfo CachedMaterialInfo;
	mutable bool bIsCachedMaterialInfoDirty = true;
};
