#include "pch.h"
#include "ParticleModuleRotation.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleRotation::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner || !Owner->Component)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleRotationPayload, Payload);

	// UniformCurve용 랜덤 비율 저장 (0~1)
	Payload.RandomFactor = Owner->RandomStream.GetFraction();

	// 초기 회전 설정 (RelativeTime = 0)
	float InitialRot = RotationOverLife.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	ParticleBase->Rotation = InitialRot;
}

void UParticleModuleRotation::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인
	const EDistributionType DistType = RotationOverLife.Type;

	// 언리얼 엔진 방식: 모든 파티클 업데이트
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleRotationPayload, Payload);

		float CurrentRotation;

		switch (DistType)
		{
		case EDistributionType::ConstantCurve:
			CurrentRotation = RotationOverLife.ConstantCurve.Eval(Particle.RelativeTime);
			break;

		case EDistributionType::UniformCurve:
			{
				float MinAtTime = RotationOverLife.MinCurve.Eval(Particle.RelativeTime);
				float MaxAtTime = RotationOverLife.MaxCurve.Eval(Particle.RelativeTime);
				CurrentRotation = FMath::Lerp(MinAtTime, MaxAtTime, Payload.RandomFactor);
			}
			break;

		case EDistributionType::Uniform:
			// Uniform: Spawn 시 결정된 랜덤 비율로 Min/Max 보간 (시간 무관)
			CurrentRotation = FMath::Lerp(RotationOverLife.MinValue, RotationOverLife.MaxValue, Payload.RandomFactor);
			break;

		default:
			// Constant: 고정값 사용
			CurrentRotation = RotationOverLife.ConstantValue;
			break;
		}

		// 회전값 업데이트 (RotationRate는 건드리지 않음)
		Particle.Rotation = CurrentRotation;
	END_UPDATE_LOOP;
}

void UParticleModuleRotation::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		// 새 필드명 우선, 이전 필드명(StartRotation)도 호환성 위해 지원
		if (FJsonSerializer::ReadObject(InOutHandle, "RotationOverLife", TempJson))
			RotationOverLife.Serialize(true, TempJson);
		else if (FJsonSerializer::ReadObject(InOutHandle, "StartRotation", TempJson))
			RotationOverLife.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		RotationOverLife.Serialize(false, TempJson);
		InOutHandle["RotationOverLife"] = TempJson;
	}
}
