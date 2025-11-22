#include "pch.h"
#include "ParticleModuleColor.h"
#include "../ParticleEmitterInstance.h"  // BEGIN_UPDATE_LOOP 매크로에서 필요
#include <random>

// 언리얼 엔진 호환: 페이로드 크기 반환
uint32 UParticleModuleColor::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleColorPayload);
}

void UParticleModuleColor::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase)
	{
		return;
	}

	// 색상 랜덤화 적용
	FLinearColor InitialColor = StartColor;
	if (ColorRandomness > 0.0f)
	{
		static std::random_device rd;
		static std::mt19937 gen(rd());
		std::uniform_real_distribution<float> dist(-ColorRandomness, ColorRandomness);

		InitialColor.R = FMath::Clamp(StartColor.R + dist(gen), 0.0f, 1.0f);
		InitialColor.G = FMath::Clamp(StartColor.G + dist(gen), 0.0f, 1.0f);
		InitialColor.B = FMath::Clamp(StartColor.B + dist(gen), 0.0f, 1.0f);
		InitialColor.A = FMath::Clamp(StartColor.A + dist(gen), 0.0f, 1.0f);
	}

	// 초기 색상 설정
	ParticleBase->Color = InitialColor;
	ParticleBase->BaseColor = InitialColor;

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
	PARTICLE_ELEMENT(FParticleColorPayload, ColorPayload);

	ColorPayload.InitialColor = InitialColor;
	ColorPayload.TargetColor = EndColor;
	ColorPayload.ColorChangeRate = 1.0f;  // 기본 변화 속도
}

// 언리얼 엔진 호환: Context를 사용한 업데이트 (페이로드 시스템)
void UParticleModuleColor::Update(FModuleUpdateContext& Context)
{
	// BEGIN_UPDATE_LOOP/END_UPDATE_LOOP 매크로 사용
	// 언리얼 엔진 표준 패턴: 역방향 순회, Freeze 자동 스킵
	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleColorPayload, ColorPayload);

		// 색상 보간 (OverLife 패턴)
		float Alpha = Particle.RelativeTime * ColorPayload.ColorChangeRate;
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		// 페이로드에 저장된 초기/목표 색상을 사용하여 보간
		Particle.Color = FLinearColor::Lerp(ColorPayload.InitialColor, ColorPayload.TargetColor, Alpha);
	END_UPDATE_LOOP
}

void UParticleModuleColor::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);
	// UPROPERTY 속성은 자동으로 직렬화됨
}
