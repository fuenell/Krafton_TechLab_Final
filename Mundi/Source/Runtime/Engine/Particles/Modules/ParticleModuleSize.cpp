#include "pch.h"
#include "ParticleModuleSize.h"
#include "Source/Runtime/Engine/Particles/ParticleEmitterInstance.h" // BEGIN_UPDATE_LOOP 매크로에서 필요
#include "ParticleSystemComponent.h"

// 언리얼 엔진 호환: 페이로드 크기 반환
uint32 UParticleModuleSize::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleSizePayload);
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
	PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

	// UniformCurve용 랜덤 비율 저장 (0~1, 각 축별)
	SizePayload.RandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);

	// 초기 크기 설정 (RelativeTime = 0)
	FVector LocalInitialSize = SizeOverLife.GetValue(0.0f, Owner->RandomStream, Owner->Component);

	// 음수 크기 방지
	LocalInitialSize.X = FMath::Max(LocalInitialSize.X, 0.01f);
	LocalInitialSize.Y = FMath::Max(LocalInitialSize.Y, 0.01f);
	LocalInitialSize.Z = FMath::Max(LocalInitialSize.Z, 0.01f);

	const float ComponentScaleX = Owner->Component->GetWorldScale().X;
	FVector FinalWorldScale = LocalInitialSize * ComponentScaleX;

	ParticleBase->Size = FinalWorldScale;
	ParticleBase->BaseSize = FinalWorldScale;
}

// 언리얼 엔진 호환: Context를 사용한 업데이트 (페이로드 시스템)
void UParticleModuleSize::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인
	const EDistributionType DistType = SizeOverLife.Type;
	const float ComponentScaleX = Context.Owner.Component->GetWorldScale().X;

	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

		FVector CurrentSizeVec;

		switch (DistType)
		{
		case EDistributionType::ConstantCurve:
			// ConstantCurve: RelativeTime에 따라 커브 평가
			CurrentSizeVec = SizeOverLife.ConstantCurve.Eval(Particle.RelativeTime);
			CurrentSizeVec = CurrentSizeVec * ComponentScaleX;
			break;

		case EDistributionType::UniformCurve:
			{
				// UniformCurve: Min/Max 커브 평가 후 저장된 랜덤 비율로 보간
				FVector MinAtTime = SizeOverLife.MinCurve.Eval(Particle.RelativeTime);
				FVector MaxAtTime = SizeOverLife.MaxCurve.Eval(Particle.RelativeTime);
				CurrentSizeVec.X = FMath::Lerp(MinAtTime.X, MaxAtTime.X, SizePayload.RandomFactor.X);
				CurrentSizeVec.Y = FMath::Lerp(MinAtTime.Y, MaxAtTime.Y, SizePayload.RandomFactor.Y);
				CurrentSizeVec.Z = FMath::Lerp(MinAtTime.Z, MaxAtTime.Z, SizePayload.RandomFactor.Z);
				CurrentSizeVec = CurrentSizeVec * ComponentScaleX;
			}
			break;

		case EDistributionType::Uniform:
			{
				// Uniform: Spawn 시 결정된 랜덤 비율로 Min/Max 보간 (시간 무관, 고정값)
				CurrentSizeVec.X = FMath::Lerp(SizeOverLife.MinValue.X, SizeOverLife.MaxValue.X, SizePayload.RandomFactor.X);
				CurrentSizeVec.Y = FMath::Lerp(SizeOverLife.MinValue.Y, SizeOverLife.MaxValue.Y, SizePayload.RandomFactor.Y);
				CurrentSizeVec.Z = FMath::Lerp(SizeOverLife.MinValue.Z, SizeOverLife.MaxValue.Z, SizePayload.RandomFactor.Z);
				CurrentSizeVec = CurrentSizeVec * ComponentScaleX;
			}
			break;

		default:
			// Constant: 고정값 사용
			CurrentSizeVec = SizeOverLife.ConstantValue * ComponentScaleX;
			break;
		}

		// 음수 크기 방지
		Particle.Size.X = FMath::Max(CurrentSizeVec.X, 0.01f);
		Particle.Size.Y = FMath::Max(CurrentSizeVec.Y, 0.01f);
		Particle.Size.Z = FMath::Max(CurrentSizeVec.Z, 0.01f);
	END_UPDATE_LOOP
}

void UParticleModuleSize::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		// 새 필드명 우선, 이전 필드명(StartSize)도 호환성 위해 지원
		if (FJsonSerializer::ReadObject(InOutHandle, "SizeOverLife", TempJson))
			SizeOverLife.Serialize(true, TempJson);
		else if (FJsonSerializer::ReadObject(InOutHandle, "StartSize", TempJson))
			SizeOverLife.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		SizeOverLife.Serialize(false, TempJson);
		InOutHandle["SizeOverLife"] = TempJson;
	}
}
