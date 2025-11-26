#include "pch.h"
#include "ParticleModuleAcceleration.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

void UParticleModuleAcceleration::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!Owner || !Owner->Component)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 페이로드 가져오기
	PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

	// UniformCurve용 랜덤 비율 저장
	Payload.RandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);

	// 중력 Z 성분 저장
	Payload.GravityZ = bApplyGravity ? (-9.8f * GravityScale) : 0.0f;
}

void UParticleModuleAcceleration::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인
	const EDistributionType DistType = AccelerationOverLife.Type;

	// 언리얼 엔진 방식: 모든 파티클 업데이트 (Context 사용)
	BEGIN_UPDATE_LOOP;
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleAccelerationPayload, Payload);

		FVector CurrentAcceleration;

		switch (DistType)
		{
		case EDistributionType::ConstantCurve:
			CurrentAcceleration = AccelerationOverLife.ConstantCurve.Eval(Particle.RelativeTime);
			break;

		case EDistributionType::UniformCurve:
			{
				FVector MinAtTime = AccelerationOverLife.MinCurve.Eval(Particle.RelativeTime);
				FVector MaxAtTime = AccelerationOverLife.MaxCurve.Eval(Particle.RelativeTime);
				CurrentAcceleration.X = FMath::Lerp(MinAtTime.X, MaxAtTime.X, Payload.RandomFactor.X);
				CurrentAcceleration.Y = FMath::Lerp(MinAtTime.Y, MaxAtTime.Y, Payload.RandomFactor.Y);
				CurrentAcceleration.Z = FMath::Lerp(MinAtTime.Z, MaxAtTime.Z, Payload.RandomFactor.Z);
			}
			break;

		case EDistributionType::Uniform:
			{
				// Uniform: Spawn 시 결정된 랜덤 비율로 Min/Max 보간 (시간 무관)
				CurrentAcceleration.X = FMath::Lerp(AccelerationOverLife.MinValue.X, AccelerationOverLife.MaxValue.X, Payload.RandomFactor.X);
				CurrentAcceleration.Y = FMath::Lerp(AccelerationOverLife.MinValue.Y, AccelerationOverLife.MaxValue.Y, Payload.RandomFactor.Y);
				CurrentAcceleration.Z = FMath::Lerp(AccelerationOverLife.MinValue.Z, AccelerationOverLife.MaxValue.Z, Payload.RandomFactor.Z);
			}
			break;

		default:
			// Constant: 고정값 사용
			CurrentAcceleration = AccelerationOverLife.ConstantValue;
			break;
		}

		// 중력 추가
		CurrentAcceleration.Z += Payload.GravityZ;

		// 속도에 가속도 적용
		Particle.Velocity += CurrentAcceleration * DeltaTime;
	END_UPDATE_LOOP;
}

void UParticleModuleAcceleration::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		// 새 필드명 우선, 이전 필드명(Acceleration)도 호환성 위해 지원
		if (FJsonSerializer::ReadObject(InOutHandle, "AccelerationOverLife", TempJson))
			AccelerationOverLife.Serialize(true, TempJson);
		else if (FJsonSerializer::ReadObject(InOutHandle, "Acceleration", TempJson))
			AccelerationOverLife.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bApplyGravity", bApplyGravity);
		FJsonSerializer::ReadFloat(InOutHandle, "GravityScale", GravityScale);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		AccelerationOverLife.Serialize(false, TempJson);
		InOutHandle["AccelerationOverLife"] = TempJson;

		InOutHandle["bApplyGravity"] = bApplyGravity;
		InOutHandle["GravityScale"] = GravityScale;
	}
}
