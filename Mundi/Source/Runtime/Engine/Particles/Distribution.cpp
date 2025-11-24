#include "pch.h"
#include "Distribution.h"
#include "Source/Runtime/Engine/Components/ParticleSystemComponent.h"

// ============================================================
// FDistributionFloat::GetValue() 구현
// ============================================================
float FDistributionFloat::GetValue(
	float Time,
	FParticleRandomStream& RandomStream,
	UParticleSystemComponent* Owner
) const
{
	switch (Type)
	{
	case EDistributionType::Constant:
		// 고정값 반환
		return ConstantValue;

	case EDistributionType::Uniform:
		// Min~Max 범위에서 랜덤 값 (결정론적 랜덤)
		return RandomStream.GetRangeFloat(MinValue, MaxValue);

	case EDistributionType::ConstantCurve:
		// 시간에 따른 커브 값 (모든 파티클 동일)
		return ConstantCurve.Eval(Time);

	case EDistributionType::UniformCurve:
	{
		// 시간에 따른 Min/Max 커브 계산 후 그 범위 내 랜덤
		float MinAtTime = MinCurve.Eval(Time);
		float MaxAtTime = MaxCurve.Eval(Time);
		return RandomStream.GetRangeFloat(MinAtTime, MaxAtTime);
	}

	case EDistributionType::ParticleParameter:
		// 런타임 파라미터에서 값 가져오기
		if (Owner && !ParameterName.empty())
		{
			return Owner->GetFloatParameter(ParameterName, ParameterDefaultValue);
		}
		return ParameterDefaultValue;

	default:
		return 0.0f;
	}
}

// ============================================================
// FDistributionVector::GetValue() 구현
// ============================================================
FVector FDistributionVector::GetValue(
	float Time,
	FParticleRandomStream& RandomStream,
	UParticleSystemComponent* Owner
) const
{
	switch (Type)
	{
	case EDistributionType::Constant:
		// 고정값 반환
		return ConstantValue;

	case EDistributionType::Uniform:
		// 각 축별로 Min~Max 범위에서 랜덤 값
		return RandomStream.GetRangeVector(MinValue, MaxValue);

	case EDistributionType::ConstantCurve:
		// 시간에 따른 커브 값
		return ConstantCurve.Eval(Time);

	case EDistributionType::UniformCurve:
	{
		// 시간에 따른 Min/Max 커브 계산 후 그 범위 내 랜덤
		FVector MinAtTime = MinCurve.Eval(Time);
		FVector MaxAtTime = MaxCurve.Eval(Time);
		return RandomStream.GetRangeVector(MinAtTime, MaxAtTime);
	}

	case EDistributionType::ParticleParameter:
		// 런타임 파라미터에서 값 가져오기
		if (Owner && !ParameterName.empty())
		{
			return Owner->GetVectorParameter(ParameterName, ParameterDefaultValue);
		}
		return ParameterDefaultValue;

	default:
		return FVector(0.0f, 0.0f, 0.0f);
	}
}
