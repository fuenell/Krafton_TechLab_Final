#pragma once

#include "ParticleModule.h"
#include "UParticleModuleRequired.generated.h"

UCLASS(DisplayName="필수 모듈", Description="이미터에 필수적인 설정을 포함하는 모듈입니다")
class UParticleModuleRequired : public UParticleModule
{
public:
	GENERATED_REFLECTION_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Required")
	FString EmitterName = "Emitter";

	UPROPERTY(EditAnywhere, Category="Required")
	class UMaterialInterface* Material = nullptr;

	// 언리얼 엔진 호환: 렌더링 정렬 방식
	UPROPERTY(EditAnywhere, Category="Required")
	int32 ScreenAlignment = 0;  // 0: Billboard, 1: Velocity aligned, 2: Axis aligned 등

	UPROPERTY(EditAnywhere, Category="Required")
	bool bOrientZAxisTowardCamera = false;  // Z축을 카메라로 향하게 할지

	UParticleModuleRequired() = default;
	virtual ~UParticleModuleRequired() = default;

	// 언리얼 엔진 호환: 렌더 스레드용 데이터로 변환
	FParticleRequiredModule ToRenderThreadData() const;

	virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
