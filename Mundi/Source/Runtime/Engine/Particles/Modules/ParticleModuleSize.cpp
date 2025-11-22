#include "pch.h"
#include "ParticleModuleSize.h"
#include "../ParticleEmitterInstance.h"  // BEGIN_UPDATE_LOOP 매크로에서 필요
#include <random>

// 언리얼 엔진 호환: 페이로드 크기 반환
uint32 UParticleModuleSize::RequiredBytes(FParticleEmitterInstance* Owner)
{
	return sizeof(FParticleSizePayload);
}

void UParticleModuleSize::Spawn(FParticleEmitterInstance* Owner, int32 Offset, float SpawnTime, FBaseParticle* ParticleBase)
{
	if (!ParticleBase)
	{
		return;
	}

	// 랜덤 크기 오프셋
	static std::random_device rd;
	static std::mt19937 gen(rd());
	std::uniform_real_distribution<float> dist(-1.0f, 1.0f);

	FVector RandomOffset(
		dist(gen) * StartSizeRange.X,
		dist(gen) * StartSizeRange.Y,
		dist(gen) * StartSizeRange.Z
	);

	FVector InitialSize = StartSize + RandomOffset;

	// 음수 크기 방지
	InitialSize.X = FMath::Max(InitialSize.X, 0.01f);
	InitialSize.Y = FMath::Max(InitialSize.Y, 0.01f);
	InitialSize.Z = FMath::Max(InitialSize.Z, 0.01f);

	ParticleBase->Size = InitialSize;
	ParticleBase->BaseSize = InitialSize;

	// 언리얼 엔진 호환: CurrentOffset 초기화
	uint32 CurrentOffset = Offset;

	// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
	PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

	SizePayload.InitialSize = InitialSize;
	SizePayload.EndSize = EndSize;
	SizePayload.SizeMultiplierOverLife = 1.0f;  // 기본 배율

	// SizeOverLife 활성화 시 Update 모듈로 동작
	if (bUseSizeOverLife)
	{
		// bUpdateModule은 const가 아니므로 동적으로 설정 가능 (실제로는 생성자에서 설정하는 것이 더 좋음)
		const_cast<UParticleModuleSize*>(this)->bUpdateModule = true;
	}
}

// 언리얼 엔진 호환: Context를 사용한 업데이트 (페이로드 시스템)
void UParticleModuleSize::Update(FModuleUpdateContext& Context)
{
	// SizeOverLife가 비활성화면 업데이트 불필요
	if (!bUseSizeOverLife)
	{
		return;
	}

	// BEGIN_UPDATE_LOOP/END_UPDATE_LOOP 매크로 사용
	// 언리얼 엔진 표준 패턴: 역방향 순회, Freeze 자동 스킵
	BEGIN_UPDATE_LOOP
		// 언리얼 엔진 호환: PARTICLE_ELEMENT 매크로 (CurrentOffset 자동 증가)
		PARTICLE_ELEMENT(FParticleSizePayload, SizePayload);

		// 크기 보간 (OverLife 패턴)
		float Alpha = Particle.RelativeTime * SizePayload.SizeMultiplierOverLife;
		Alpha = FMath::Clamp(Alpha, 0.0f, 1.0f);

		// 페이로드에 저장된 초기/목표 크기를 사용하여 보간
		Particle.Size.X = FMath::Lerp(SizePayload.InitialSize.X, SizePayload.EndSize.X, Alpha);
		Particle.Size.Y = FMath::Lerp(SizePayload.InitialSize.Y, SizePayload.EndSize.Y, Alpha);
		Particle.Size.Z = FMath::Lerp(SizePayload.InitialSize.Z, SizePayload.EndSize.Z, Alpha);
	END_UPDATE_LOOP
}

void UParticleModuleSize::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);
	// UPROPERTY 속성은 자동으로 직렬화됨
}
