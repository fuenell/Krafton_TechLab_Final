#include "pch.h"
#include "ParticleModuleColor.h"
#include "ParticleEmitterInstance.h"  // BEGIN_UPDATE_LOOP 매크로에서 필요

// 언리얼 엔진 호환: 페이로드 크기 반환
uint32 UParticleModuleColor::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleColorPayload);
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase || !Owner)
	{
		return;
	}

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
	PARTICLE_ELEMENT(FParticleColorPayload, ColorPayload);

	// UniformCurve용 랜덤 비율 저장 (0~1)
	ColorPayload.RGBRandomFactor = FVector(
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction(),
		Owner->RandomStream.GetFraction()
	);
	ColorPayload.AlphaRandomFactor = Owner->RandomStream.GetFraction();

	// 초기 색상 설정 (RelativeTime = 0)
	FLinearColor InitialColor = ColorOverLife.GetValue(0.0f, Owner->RandomStream, Owner->Component);
	ParticleBase->Color = InitialColor;
	ParticleBase->BaseColor = InitialColor;
}

// 언리얼 엔진 호환: Context를 사용한 업데이트 (페이로드 시스템)
void UParticleModuleColor::Update(FModuleUpdateContext& Context)
{
	// Distribution 타입 확인 (RGB와 Alpha 각각)
	const EDistributionType RGBDistType = ColorOverLife.RGB.Type;
	const EDistributionType AlphaDistType = ColorOverLife.Alpha.Type;

	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleColorPayload, ColorPayload);

		FLinearColor CurrentColor;

		// RGB 처리
		switch (RGBDistType)
		{
		case EDistributionType::ConstantCurve:
			{
				FVector RGB = ColorOverLife.RGB.ConstantCurve.Eval(Particle.RelativeTime);
				CurrentColor.R = RGB.X;
				CurrentColor.G = RGB.Y;
				CurrentColor.B = RGB.Z;
			}
			break;

		case EDistributionType::UniformCurve:
			{
				FVector MinRGB = ColorOverLife.RGB.MinCurve.Eval(Particle.RelativeTime);
				FVector MaxRGB = ColorOverLife.RGB.MaxCurve.Eval(Particle.RelativeTime);
				CurrentColor.R = FMath::Lerp(MinRGB.X, MaxRGB.X, ColorPayload.RGBRandomFactor.X);
				CurrentColor.G = FMath::Lerp(MinRGB.Y, MaxRGB.Y, ColorPayload.RGBRandomFactor.Y);
				CurrentColor.B = FMath::Lerp(MinRGB.Z, MaxRGB.Z, ColorPayload.RGBRandomFactor.Z);
			}
			break;

		case EDistributionType::Uniform:
			{
				// Uniform: Spawn 시 결정된 랜덤 비율로 Min/Max 보간 (시간 무관)
				CurrentColor.R = FMath::Lerp(ColorOverLife.RGB.MinValue.X, ColorOverLife.RGB.MaxValue.X, ColorPayload.RGBRandomFactor.X);
				CurrentColor.G = FMath::Lerp(ColorOverLife.RGB.MinValue.Y, ColorOverLife.RGB.MaxValue.Y, ColorPayload.RGBRandomFactor.Y);
				CurrentColor.B = FMath::Lerp(ColorOverLife.RGB.MinValue.Z, ColorOverLife.RGB.MaxValue.Z, ColorPayload.RGBRandomFactor.Z);
			}
			break;

		default:
			// Constant 타입: 고정값 사용
			{
				CurrentColor.R = ColorOverLife.RGB.ConstantValue.X;
				CurrentColor.G = ColorOverLife.RGB.ConstantValue.Y;
				CurrentColor.B = ColorOverLife.RGB.ConstantValue.Z;
			}
			break;
		}

		// Alpha 처리
		switch (AlphaDistType)
		{
		case EDistributionType::ConstantCurve:
			CurrentColor.A = ColorOverLife.Alpha.ConstantCurve.Eval(Particle.RelativeTime);
			break;

		case EDistributionType::UniformCurve:
			{
				float MinA = ColorOverLife.Alpha.MinCurve.Eval(Particle.RelativeTime);
				float MaxA = ColorOverLife.Alpha.MaxCurve.Eval(Particle.RelativeTime);
				CurrentColor.A = FMath::Lerp(MinA, MaxA, ColorPayload.AlphaRandomFactor);
			}
			break;

		case EDistributionType::Uniform:
			// Uniform: Spawn 시 결정된 랜덤 비율로 Min/Max 보간 (시간 무관)
			CurrentColor.A = FMath::Lerp(ColorOverLife.Alpha.MinValue, ColorOverLife.Alpha.MaxValue, ColorPayload.AlphaRandomFactor);
			break;

		default:
			// Constant 타입: 고정값 사용
			CurrentColor.A = ColorOverLife.Alpha.ConstantValue;
			break;
		}

		Particle.Color = CurrentColor;
	END_UPDATE_LOOP
}

void UParticleModuleColor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		// 새 필드명 우선, 이전 필드명(StartColor)도 호환성 위해 지원
		if (FJsonSerializer::ReadObject(InOutHandle, "ColorOverLife", TempJson))
			ColorOverLife.Serialize(true, TempJson);
		else if (FJsonSerializer::ReadObject(InOutHandle, "StartColor", TempJson))
			ColorOverLife.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		ColorOverLife.Serialize(false, TempJson);
		InOutHandle["ColorOverLife"] = TempJson;
	}
}
