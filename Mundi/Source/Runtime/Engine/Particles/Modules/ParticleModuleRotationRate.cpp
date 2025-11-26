#include "pch.h"
#include "ParticleModuleRotationRate.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleRotationRate::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner || !Owner->Component)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleRotationRatePayload, Payload);

	// UniformCurve용 랜덤 비율 저장 (0~1)
	Payload.RandomFactor = Owner->RandomStream.GetFraction();

	// 초기 회전 속도 설정 (RelativeTime = 0)
	float InitialRate = RotationRateOverLife.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	ParticleBase->RotationRate = InitialRate;
	ParticleBase->BaseRotationRate = InitialRate;
}

void UParticleModuleRotationRate::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인
	const EDistributionType DistType = RotationRateOverLife.Type;

	// 언리얼 엔진 방식: 모든 파티클 업데이트
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleRotationRatePayload, Payload);

		float CurrentRotationRate;

		switch (DistType)
		{
		case EDistributionType::ConstantCurve:
			CurrentRotationRate = RotationRateOverLife.ConstantCurve.Eval(Particle.RelativeTime);
			break;

		case EDistributionType::UniformCurve:
			{
				float MinAtTime = RotationRateOverLife.MinCurve.Eval(Particle.RelativeTime);
				float MaxAtTime = RotationRateOverLife.MaxCurve.Eval(Particle.RelativeTime);
				CurrentRotationRate = FMath::Lerp(MinAtTime, MaxAtTime, Payload.RandomFactor);
			}
			break;

		case EDistributionType::Uniform:
			// Uniform: Spawn 시 결정된 랜덤 비율로 Min/Max 보간 (시간 무관)
			CurrentRotationRate = FMath::Lerp(RotationRateOverLife.MinValue, RotationRateOverLife.MaxValue, Payload.RandomFactor);
			break;

		default:
			// Constant: 고정값 사용
			CurrentRotationRate = RotationRateOverLife.ConstantValue;
			break;
		}

		// 회전 속도 업데이트
		Particle.RotationRate = CurrentRotationRate;
	END_UPDATE_LOOP;
}

void UParticleModuleRotationRate::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		// 새 필드명 우선, 이전 필드명(StartRotationRate)도 호환성 위해 지원
		if (FJsonSerializer::ReadObject(InOutHandle, "RotationRateOverLife", TempJson))
			RotationRateOverLife.Serialize(true, TempJson);
		else if (FJsonSerializer::ReadObject(InOutHandle, "StartRotationRate", TempJson))
			RotationRateOverLife.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		RotationRateOverLife.Serialize(false, TempJson);
		InOutHandle["RotationRateOverLife"] = TempJson;
	}
}
