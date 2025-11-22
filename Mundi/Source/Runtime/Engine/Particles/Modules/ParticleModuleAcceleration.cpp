#include "pch.h"
#include "ParticleModuleAcceleration.h"
#include "../ParticleEmitterInstance.h"
#include <cstdlib>

void UParticleModuleAcceleration::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

	// 기본 가속도 계산 (중력 포함)
	FVector BaseAcceleration = Acceleration;
	if (bApplyGravity)
	{
		const float GravityZ = -980.0f; // cm/s^2
		BaseAcceleration.Z += GravityZ * GravityScale;
	}

	// 초기 가속도 저장
	Payload.InitialAcceleration = BaseAcceleration;

	// 랜덤 오프셋 계산 (각 축별로 -AccelerationRandomness ~ +AccelerationRandomness)
	Payload.AccelerationRandomOffset = FVector(
		(static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * AccelerationRandomness.X,
		(static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * AccelerationRandomness.Y,
		(static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * AccelerationRandomness.Z
	);

	// AccelerationOverLife 초기화
	Payload.AccelerationMultiplierOverLife = AccelerationMultiplierAtStart;
}

void UParticleModuleAcceleration::Update(FModuleUpdateContext& Context)
{
	// 언리얼 엔진 방식: 모든 파티클 업데이트 (Context 사용)
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

		// AccelerationOverLife 계산
		float CurrentMultiplier = Payload.AccelerationMultiplierOverLife;
		if (bUseAccelerationOverLife)
		{
			// 수명에 따라 선형 보간 (Start -> End)
			CurrentMultiplier = AccelerationMultiplierAtStart +
				(AccelerationMultiplierAtEnd - AccelerationMultiplierAtStart) * Particle.RelativeTime;
			Payload.AccelerationMultiplierOverLife = CurrentMultiplier;
		}

		// 최종 가속도 = (초기 가속도 + 랜덤 오프셋) * OverLife 배율
		FVector FinalAcceleration = (Payload.InitialAcceleration + Payload.AccelerationRandomOffset) * CurrentMultiplier;

		// 속도에 가속도 적용
		Particle.Velocity += FinalAcceleration * DeltaTime;
	END_UPDATE_LOOP;
}

void UParticleModuleAcceleration::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);
	// UPROPERTY 속성은 자동으로 직렬화됨
}
