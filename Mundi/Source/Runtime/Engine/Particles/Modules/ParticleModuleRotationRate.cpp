#include "pch.h"
#include "ParticleModuleRotationRate.h"
#include "../ParticleEmitterInstance.h"
#include <cstdlib>

void UParticleModuleRotationRate::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleRotationRatePayload, Payload);

	// 초기 회전 속도 저장
	Payload.InitialRotationRate = StartRotationRate;

	// 랜덤 오프셋 계산 (-RotationRateRandomness ~ +RotationRateRandomness)
	Payload.RotationRateRandomOffset = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * RotationRateRandomness;

	// 목표 회전 속도 설정 (RotationRateOverLife용)
	Payload.TargetRotationRate = EndRotationRate;

	// 파티클의 초기 회전 속도 설정
	ParticleBase->RotationRate = Payload.InitialRotationRate + Payload.RotationRateRandomOffset;
	ParticleBase->BaseRotationRate = ParticleBase->RotationRate;

	// RotationRateOverLife 활성화 시 Update 모듈로 등록
	if (bUseRotationRateOverLife)
	{
		bUpdateModule = true;
	}
}

void UParticleModuleRotationRate::Update(FModuleUpdateContext& Context)
{
	// RotationRateOverLife가 비활성화되어 있으면 업데이트 불필요
	if (!bUseRotationRateOverLife)
	{
		return;
	}

	// 언리얼 엔진 방식: 모든 파티클 업데이트
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleRotationRatePayload, Payload);

		// 수명에 따라 회전 속도 선형 보간 (InitialRotationRate -> TargetRotationRate)
		float InterpolatedRotationRate = Payload.InitialRotationRate + Payload.RotationRateRandomOffset +
			(Payload.TargetRotationRate - (Payload.InitialRotationRate + Payload.RotationRateRandomOffset)) * Particle.RelativeTime;

		// 회전 속도 업데이트
		Particle.RotationRate = InterpolatedRotationRate;
	END_UPDATE_LOOP;
}

void UParticleModuleRotationRate::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	// UPROPERTY 속성은 리플렉션 시스템에 의해 자동으로 직렬화됨
}
