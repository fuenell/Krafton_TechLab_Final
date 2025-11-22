#include "pch.h"
#include "ParticleModuleRotation.h"
#include "../ParticleEmitterInstance.h"
#include <cstdlib>

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleRotationPayload, Payload);

	// 초기 회전값 저장
	Payload.InitialRotation = StartRotation;

	// 랜덤 오프셋 계산 (-RotationRandomness ~ +RotationRandomness)
	Payload.RotationRandomOffset = (static_cast<float>(rand()) / RAND_MAX * 2.0f - 1.0f) * RotationRandomness;

	// 목표 회전값 설정 (RotationOverLife용)
	Payload.TargetRotation = EndRotation;

	// 파티클의 초기 회전 설정
	ParticleBase->Rotation = Payload.InitialRotation + Payload.RotationRandomOffset;

	// RotationOverLife 활성화 시 Update 모듈로 등록
	if (bUseRotationOverLife)
	{
		bUpdateModule = true;
	}
}

void UParticleModuleRotation::Update(FModuleUpdateContext& Context)
{
	// RotationOverLife가 비활성화되어 있으면 업데이트 불필요
	if (!bUseRotationOverLife)
	{
		return;
	}

	// 언리얼 엔진 방식: 모든 파티클 업데이트
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleRotationPayload, Payload);

		// 수명에 따라 회전값 선형 보간 (InitialRotation -> TargetRotation)
		float InterpolatedRotation = Payload.InitialRotation + Payload.RotationRandomOffset +
			(Payload.TargetRotation - (Payload.InitialRotation + Payload.RotationRandomOffset)) * Particle.RelativeTime;

		// 회전값 업데이트 (RotationRate는 건드리지 않음)
		Particle.Rotation = InterpolatedRotation;
	END_UPDATE_LOOP;
}

void UParticleModuleRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	// UPROPERTY 속성은 리플렉션 시스템에 의해 자동으로 직렬화됨
}
