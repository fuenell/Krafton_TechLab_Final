#pragma once

#include "Vector.h"
#include "Color.h"
#include "ParticleRandomStream.h"

// Forward declarations
class UParticleSystemComponent;

// 언리얼 엔진 호환: Distribution 시스템
// 파티클 모듈의 속성값을 유연하게 제어 (상수, 균등 분포, 커브 등)

// ============================================================
// InterpCurve 보간 모드
// ============================================================
enum class EInterpCurveMode : uint8
{
	Linear,          // 선형 보간
	Constant,        // 계단식 (보간 없음, 이전 값 유지)
	Curve,           // 커브 보간 (접선 사용) - 추후 구현
	CurveAuto,       // 자동 접선 - 추후 구현
	CurveAutoClamped // 클램핑된 자동 접선 - 추후 구현
};

// ============================================================
// InterpCurve 키프레임 (Float 버전)
// ============================================================
struct FInterpCurvePointFloat
{
	float InVal;           // 시간 (X축)
	float OutVal;          // 값 (Y축)
	float ArriveTangent;   // 들어오는 접선 (Curve 모드용)
	float LeaveTangent;    // 나가는 접선 (Curve 모드용)
	EInterpCurveMode InterpMode;  // 보간 모드

	FInterpCurvePointFloat()
		: InVal(0.0f)
		, OutVal(0.0f)
		, ArriveTangent(0.0f)
		, LeaveTangent(0.0f)
		, InterpMode(EInterpCurveMode::Linear)
	{
	}

	FInterpCurvePointFloat(float InTime, float InValue, EInterpCurveMode InMode = EInterpCurveMode::Linear)
		: InVal(InTime)
		, OutVal(InValue)
		, ArriveTangent(0.0f)
		, LeaveTangent(0.0f)
		, InterpMode(InMode)
	{
	}
};

// ============================================================
// InterpCurve 키프레임 (Vector 버전)
// ============================================================
struct FInterpCurvePointVector
{
	float InVal;           // 시간 (X축)
	FVector OutVal;        // 값 (Y축)
	FVector ArriveTangent; // 들어오는 접선
	FVector LeaveTangent;  // 나가는 접선
	EInterpCurveMode InterpMode;

	FInterpCurvePointVector()
		: InVal(0.0f)
		, OutVal(0.0f, 0.0f, 0.0f)
		, ArriveTangent(0.0f, 0.0f, 0.0f)
		, LeaveTangent(0.0f, 0.0f, 0.0f)
		, InterpMode(EInterpCurveMode::Linear)
	{
	}

	FInterpCurvePointVector(float InTime, const FVector& InValue, EInterpCurveMode InMode = EInterpCurveMode::Linear)
		: InVal(InTime)
		, OutVal(InValue)
		, ArriveTangent(0.0f, 0.0f, 0.0f)
		, LeaveTangent(0.0f, 0.0f, 0.0f)
		, InterpMode(InMode)
	{
	}
};

// ============================================================
// Float InterpCurve
// ============================================================
struct FInterpCurveFloat
{
	TArray<FInterpCurvePointFloat> Points;
	bool bIsLooped;
	float LoopKeyOffset;

	FInterpCurveFloat()
		: bIsLooped(false)
		, LoopKeyOffset(0.0f)
	{
	}

	// 시간에 따른 값 계산 (선형 보간만 지원)
	float Eval(float Time) const
	{
		if (Points.IsEmpty())
		{
			return 0.0f;
		}

		if (Points.Num() == 1)
		{
			return Points[0].OutVal;
		}

		// 시간이 첫 키프레임 이전
		if (Time <= Points[0].InVal)
		{
			return Points[0].OutVal;
		}

		// 시간이 마지막 키프레임 이후
		if (Time >= Points[Points.Num() - 1].InVal)
		{
			return Points[Points.Num() - 1].OutVal;
		}

		// 키프레임 구간 찾기
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			const FInterpCurvePointFloat& Point1 = Points[i];
			const FInterpCurvePointFloat& Point2 = Points[i + 1];

			if (Time >= Point1.InVal && Time <= Point2.InVal)
			{
				// 보간 계수 계산
				float Alpha = (Time - Point1.InVal) / (Point2.InVal - Point1.InVal);

				// 보간 모드에 따라 값 계산
				if (Point1.InterpMode == EInterpCurveMode::Constant)
				{
					return Point1.OutVal;
				}
				else // Linear (기본)
				{
					return Point1.OutVal + (Point2.OutVal - Point1.OutVal) * Alpha;
				}
			}
		}

		return 0.0f;
	}

	// 키프레임 추가
	void AddPoint(float Time, float Value, EInterpCurveMode Mode = EInterpCurveMode::Linear)
	{
		Points.Add(FInterpCurvePointFloat(Time, Value, Mode));
	}
};

// ============================================================
// Vector InterpCurve
// ============================================================
struct FInterpCurveVector
{
	TArray<FInterpCurvePointVector> Points;
	bool bIsLooped;
	float LoopKeyOffset;

	FInterpCurveVector()
		: bIsLooped(false)
		, LoopKeyOffset(0.0f)
	{
	}

	// 시간에 따른 값 계산 (선형 보간만 지원)
	FVector Eval(float Time) const
	{
		if (Points.IsEmpty())
		{
			return FVector(0.0f, 0.0f, 0.0f);
		}

		if (Points.Num() == 1)
		{
			return Points[0].OutVal;
		}

		// 시간이 첫 키프레임 이전
		if (Time <= Points[0].InVal)
		{
			return Points[0].OutVal;
		}

		// 시간이 마지막 키프레임 이후
		if (Time >= Points[Points.Num() - 1].InVal)
		{
			return Points[Points.Num() - 1].OutVal;
		}

		// 키프레임 구간 찾기
		for (int32 i = 0; i < Points.Num() - 1; ++i)
		{
			const FInterpCurvePointVector& Point1 = Points[i];
			const FInterpCurvePointVector& Point2 = Points[i + 1];

			if (Time >= Point1.InVal && Time <= Point2.InVal)
			{
				// 보간 계수 계산
				float Alpha = (Time - Point1.InVal) / (Point2.InVal - Point1.InVal);

				// 보간 모드에 따라 값 계산
				if (Point1.InterpMode == EInterpCurveMode::Constant)
				{
					return Point1.OutVal;
				}
				else // Linear (기본)
				{
					return FVector(
						Point1.OutVal.X + (Point2.OutVal.X - Point1.OutVal.X) * Alpha,
						Point1.OutVal.Y + (Point2.OutVal.Y - Point1.OutVal.Y) * Alpha,
						Point1.OutVal.Z + (Point2.OutVal.Z - Point1.OutVal.Z) * Alpha
					);
				}
			}
		}

		return FVector(0.0f, 0.0f, 0.0f);
	}

	// 키프레임 추가
	void AddPoint(float Time, const FVector& Value, EInterpCurveMode Mode = EInterpCurveMode::Linear)
	{
		Points.Add(FInterpCurvePointVector(Time, Value, Mode));
	}
};

// ============================================================
// Distribution 타입
// ============================================================
enum class EDistributionType : uint8
{
	Constant,           // 고정값
	Uniform,            // 균등 분포 (Min ~ Max)
	ConstantCurve,      // 시간에 따른 커브 (모든 파티클 동일)
	UniformCurve,       // 시간에 따른 Min~Max 커브
	ParticleParameter   // 런타임 동적 제어 (게임 코드에서 설정)
};

// ============================================================
// Float Distribution (수명, 속도 크기 등)
// ============================================================
struct FDistributionFloat
{
	EDistributionType Type = EDistributionType::Constant;

	// Constant 모드
	float ConstantValue = 0.0f;

	// Uniform 모드 (Min ~ Max 랜덤)
	float MinValue = 0.0f;
	float MaxValue = 1.0f;

	// ConstantCurve 모드 (시간에 따른 단일 커브)
	FInterpCurveFloat ConstantCurve;

	// UniformCurve 모드 (시간에 따른 Min~Max 커브)
	FInterpCurveFloat MinCurve;
	FInterpCurveFloat MaxCurve;

	// ParticleParameter 모드 (런타임 제어)
	FString ParameterName = "";        // 파라미터 이름 (예: "SpawnRate")
	float ParameterDefaultValue = 0.0f; // 파라미터가 없을 때 기본값

	// 생성자
	FDistributionFloat()
		: Type(EDistributionType::Constant)
		, ConstantValue(0.0f)
		, MinValue(0.0f)
		, MaxValue(1.0f)
		, ParameterDefaultValue(0.0f)
	{
	}

	explicit FDistributionFloat(float InConstant)
		: Type(EDistributionType::Constant)
		, ConstantValue(InConstant)
		, MinValue(InConstant)
		, MaxValue(InConstant)
		, ParameterDefaultValue(InConstant)
	{
	}

	FDistributionFloat(float InMin, float InMax)
		: Type(EDistributionType::Uniform)
		, ConstantValue(InMin)
		, MinValue(InMin)
		, MaxValue(InMax)
		, ParameterDefaultValue(InMin)
	{
	}

	// 값 가져오기 (언리얼 엔진 호환 시그니처)
	float GetValue(
		float Time,                          // 파티클 나이 (RelativeTime: 0.0~1.0) 또는 이미터 시간
		FParticleRandomStream& RandomStream, // 결정론적 랜덤 스트림
		UParticleSystemComponent* Owner = nullptr  // 파라미터 조회용 (nullable)
	) const;

	// 정적 생성 헬퍼
	static FDistributionFloat MakeConstant(float Value)
	{
		FDistributionFloat Dist;
		Dist.Type = EDistributionType::Constant;
		Dist.ConstantValue = Value;
		return Dist;
	}

	static FDistributionFloat MakeUniform(float Min, float Max)
	{
		FDistributionFloat Dist;
		Dist.Type = EDistributionType::Uniform;
		Dist.MinValue = Min;
		Dist.MaxValue = Max;
		return Dist;
	}

	static FDistributionFloat MakeParameter(const FString& ParamName, float DefaultValue = 0.0f)
	{
		FDistributionFloat Dist;
		Dist.Type = EDistributionType::ParticleParameter;
		Dist.ParameterName = ParamName;
		Dist.ParameterDefaultValue = DefaultValue;
		return Dist;
	}
};

// ============================================================
// Vector Distribution (위치, 속도, 크기 등)
// ============================================================
struct FDistributionVector
{
	EDistributionType Type = EDistributionType::Constant;

	// Constant 모드
	FVector ConstantValue = FVector(0.0f, 0.0f, 0.0f);

	// Uniform 모드 (각 축별로 Min ~ Max 랜덤)
	FVector MinValue = FVector(0.0f, 0.0f, 0.0f);
	FVector MaxValue = FVector(1.0f, 1.0f, 1.0f);

	// ConstantCurve 모드
	FInterpCurveVector ConstantCurve;

	// UniformCurve 모드
	FInterpCurveVector MinCurve;
	FInterpCurveVector MaxCurve;

	// ParticleParameter 모드
	FString ParameterName = "";
	FVector ParameterDefaultValue = FVector(0.0f, 0.0f, 0.0f);

	// 생성자
	FDistributionVector()
		: Type(EDistributionType::Constant)
		, ConstantValue(FVector(0.0f, 0.0f, 0.0f))
		, MinValue(FVector(0.0f, 0.0f, 0.0f))
		, MaxValue(FVector(1.0f, 1.0f, 1.0f))
		, ParameterDefaultValue(FVector(0.0f, 0.0f, 0.0f))
	{
	}

	explicit FDistributionVector(const FVector& InConstant)
		: Type(EDistributionType::Constant)
		, ConstantValue(InConstant)
		, MinValue(InConstant)
		, MaxValue(InConstant)
		, ParameterDefaultValue(InConstant)
	{
	}

	FDistributionVector(const FVector& InMin, const FVector& InMax)
		: Type(EDistributionType::Uniform)
		, ConstantValue(InMin)
		, MinValue(InMin)
		, MaxValue(InMax)
		, ParameterDefaultValue(InMin)
	{
	}

	// 값 가져오기 (언리얼 엔진 호환 시그니처)
	FVector GetValue(
		float Time,
		FParticleRandomStream& RandomStream,
		UParticleSystemComponent* Owner = nullptr
	) const;

	// 정적 생성 헬퍼
	static FDistributionVector MakeConstant(const FVector& Value)
	{
		FDistributionVector Dist;
		Dist.Type = EDistributionType::Constant;
		Dist.ConstantValue = Value;
		return Dist;
	}

	static FDistributionVector MakeUniform(const FVector& Min, const FVector& Max)
	{
		FDistributionVector Dist;
		Dist.Type = EDistributionType::Uniform;
		Dist.MinValue = Min;
		Dist.MaxValue = Max;
		return Dist;
	}

	static FDistributionVector MakeParameter(const FString& ParamName, const FVector& DefaultValue = FVector(0.0f, 0.0f, 0.0f))
	{
		FDistributionVector Dist;
		Dist.Type = EDistributionType::ParticleParameter;
		Dist.ParameterName = ParamName;
		Dist.ParameterDefaultValue = DefaultValue;
		return Dist;
	}
};

// ============================================================
// Color Distribution (언리얼 방식: RGB는 Vector, Alpha는 Float)
// ============================================================
struct FDistributionColor
{
	// 언리얼 엔진 호환: RGB와 Alpha를 분리하여 제어
	FDistributionVector RGB;    // R, G, B는 Vector Distribution (3D)
	FDistributionFloat Alpha;   // A는 Float Distribution (1D)

	// 생성자
	FDistributionColor()
	{
		// 기본값: 흰색 불투명 (1, 1, 1, 1)
		RGB = FDistributionVector::MakeConstant(FVector(1.0f, 1.0f, 1.0f));
		Alpha = FDistributionFloat::MakeConstant(1.0f);
	}

	explicit FDistributionColor(const FLinearColor& InConstant)
	{
		RGB = FDistributionVector::MakeConstant(FVector(InConstant.R, InConstant.G, InConstant.B));
		Alpha = FDistributionFloat::MakeConstant(InConstant.A);
	}

	FDistributionColor(const FLinearColor& InMin, const FLinearColor& InMax)
	{
		RGB = FDistributionVector::MakeUniform(
			FVector(InMin.R, InMin.G, InMin.B),
			FVector(InMax.R, InMax.G, InMax.B)
		);
		Alpha = FDistributionFloat::MakeUniform(InMin.A, InMax.A);
	}

	// 값 가져오기
	FLinearColor GetValue(
		float Time,
		FParticleRandomStream& RandomStream,
		UParticleSystemComponent* Owner = nullptr
	) const
	{
		FVector RGBValue = RGB.GetValue(Time, RandomStream, Owner);
		float AlphaValue = Alpha.GetValue(Time, RandomStream, Owner);

		return FLinearColor(RGBValue.X, RGBValue.Y, RGBValue.Z, AlphaValue);
	}

	// 정적 생성 헬퍼
	static FDistributionColor MakeConstant(const FLinearColor& Value)
	{
		FDistributionColor Dist;
		Dist.RGB = FDistributionVector::MakeConstant(FVector(Value.R, Value.G, Value.B));
		Dist.Alpha = FDistributionFloat::MakeConstant(Value.A);
		return Dist;
	}

	static FDistributionColor MakeUniform(const FLinearColor& Min, const FLinearColor& Max)
	{
		FDistributionColor Dist;
		Dist.RGB = FDistributionVector::MakeUniform(
			FVector(Min.R, Min.G, Min.B),
			FVector(Max.R, Max.G, Max.B)
		);
		Dist.Alpha = FDistributionFloat::MakeUniform(Min.A, Max.A);
		return Dist;
	}
};
