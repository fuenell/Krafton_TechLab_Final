#include "pch.h"
#include "ParticleModuleRequired.h"
#include "../ParticleDefinitions.h"

// 언리얼 엔진 호환: 렌더 스레드용 데이터로 변환
FParticleRequiredModule UParticleModuleRequired::ToRenderThreadData() const
{
	FParticleRequiredModule RenderData;
	RenderData.Material = Material;
	RenderData.EmitterName = EmitterName;
	RenderData.ScreenAlignment = ScreenAlignment;
	RenderData.bOrientZAxisTowardCamera = bOrientZAxisTowardCamera;
	return RenderData;
}

void UParticleModuleRequired::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);
	// UPROPERTY 속성은 자동으로 직렬화됨
}
