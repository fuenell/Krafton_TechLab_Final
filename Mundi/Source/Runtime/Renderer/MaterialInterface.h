#pragma once
#include "ResourceBase.h"
#include "Enums.h"

class UShader;
class UTexture;

// 텍스처 슬롯을 명확하게 구분하기 위한 Enum (선택 사항이지만 권장)
enum class EMaterialTextureSlot : uint8
{
	Diffuse = 0,
	Normal,
	//Specular,
	//Emissive,
	// ... 기타 슬롯 ...
	Max // 배열 크기 지정용
};

// 머티리얼 인터페이스 (추상 클래스)
class UMaterialInterface : public UResourceBase
{
	DECLARE_CLASS(UMaterialInterface, UResourceBase)
public:
	virtual UShader* GetShader() = 0;
	virtual UTexture* GetTexture(EMaterialTextureSlot Slot) const = 0;
	virtual bool HasTexture(EMaterialTextureSlot Slot) const = 0;
	virtual const FMaterialInfo& GetMaterialInfo() const = 0;
	virtual const TArray<FShaderMacro> GetShaderMacros() const = 0;
};
