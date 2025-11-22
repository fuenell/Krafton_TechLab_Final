#include "pch.h"
#include "ParticleEmitterInstance.h"
#include "Modules/ParticleModule.h"
#include "Modules/ParticleModuleSpawn.h"
#include <cstring>

FParticleEmitterInstance::FParticleEmitterInstance()
	: SpriteTemplate(nullptr)
	, Component(nullptr)
	, CurrentLODLevelIndex(0)
	, CurrentLODLevel(nullptr)
	, ParticleData(nullptr)
	, ParticleIndices(nullptr)
	, InstanceData(nullptr)
	, InstancePayloadSize(0)
	, PayloadOffset(0)
	, ParticleSize(0)
	, ParticleStride(0)
	, ActiveParticles(0)
	, ParticleCounter(0)
	, MaxActiveParticles(0)
	, SpawnFraction(0.0f)
	, bBurstFired(false)  // 언리얼 엔진 호환: Burst 초기화
{
}

FParticleEmitterInstance::~FParticleEmitterInstance()
{
	// 언리얼 엔진 호환: FParticleDataContainer가 자동으로 메모리 해제
	// ParticleDataContainer.Free()는 소멸자에서 자동 호출됨
	ParticleData = nullptr;
	ParticleIndices = nullptr;

	if (InstanceData)
	{
		delete[] InstanceData;
		InstanceData = nullptr;
	}
}

void FParticleEmitterInstance::Init(UParticleSystemComponent* InComponent, UParticleEmitter* InTemplate)
{
	Component = InComponent;
	SpriteTemplate = InTemplate;

	if (SpriteTemplate)
	{
		// 현재 LOD 레벨 가져오기 (기본값 0)
		CurrentLODLevelIndex = 0;
		CurrentLODLevel = SpriteTemplate->GetLODLevel(CurrentLODLevelIndex);

		if (CurrentLODLevel)
		{
			// 언리얼 엔진 호환: 페이로드 크기 계산
			// 각 모듈이 필요로 하는 추가 데이터 크기를 계산하고 오프셋 할당
			uint32 TotalPayloadSize = 0;
			InstancePayloadSize = 0;

			// 모든 모듈의 페이로드 크기 계산
			for (UParticleModule* Module : CurrentLODLevel->Modules)
			{
				if (Module && Module->bEnabled)
				{
					// 파티클별 페이로드
					uint32 ModulePayloadSize = Module->RequiredBytes(this);
					Module->ModuleOffsetInParticle = TotalPayloadSize;
					TotalPayloadSize += ModulePayloadSize;

					// 인스턴스별 페이로드
					InstancePayloadSize += Module->RequiredBytesPerInstance();
				}
			}

			// 파티클 크기와 스트라이드 계산 (기본 파티클 + 페이로드)
			ParticleSize = SpriteTemplate->ParticleSize;
			PayloadOffset = ParticleSize;  // 페이로드는 기본 파티클 뒤에 위치
			ParticleStride = ParticleSize + TotalPayloadSize;

			// 인스턴스 데이터 할당 (필요한 경우)
			if (InstancePayloadSize > 0)
			{
				if (InstanceData)
				{
					delete[] InstanceData;
				}
				InstanceData = new uint8[InstancePayloadSize];
				memset(InstanceData, 0, InstancePayloadSize);
			}

			// 초기 파티클 데이터 할당
			Resize(100); // Default to 100 particles
		}
	}
}

void FParticleEmitterInstance::Resize(int32 NewMaxActiveParticles)
{
	if (NewMaxActiveParticles == MaxActiveParticles)
	{
		return;
	}

	MaxActiveParticles = NewMaxActiveParticles;

	if (MaxActiveParticles > 0)
	{
		// 언리얼 엔진 호환: FParticleDataContainer를 사용하여 16바이트 정렬 메모리 할당
		int32 ParticleDataSize = MaxActiveParticles * ParticleStride;
		ParticleDataContainer.Alloc(ParticleDataSize, MaxActiveParticles);

		// 컨테이너에서 포인터 가져오기
		ParticleData = ParticleDataContainer.ParticleData;
		ParticleIndices = ParticleDataContainer.ParticleIndices;

		// 인덱스 초기화
		if (ParticleIndices)
		{
			for (int32 i = 0; i < MaxActiveParticles; i++)
			{
				ParticleIndices[i] = i;
			}
		}
	}
	else
	{
		// 크기가 0이면 해제
		ParticleDataContainer.Free();
		ParticleData = nullptr;
		ParticleIndices = nullptr;
	}

	ActiveParticles = 0;
}

void FParticleEmitterInstance::Tick(float DeltaTime, bool bSuppressSpawning)
{
	if (!CurrentLODLevel)
	{
		return;
	}

	// 기존 파티클 업데이트
	UpdateParticles(DeltaTime);

	// 언리얼 엔진 호환: 새 파티클 생성 (억제되지 않은 경우)
	if (!bSuppressSpawning)
	{
		// SpawnModule 찾기
		UParticleModuleSpawn* SpawnModule = nullptr;
		for (UParticleModule* Module : CurrentLODLevel->SpawnModules)
		{
			if (Module && Module->bEnabled)
			{
				SpawnModule = dynamic_cast<UParticleModuleSpawn*>(Module);
				if (SpawnModule)
				{
					break;
				}
			}
		}

		// 언리얼 엔진 호환: 스폰 로직을 모듈에 위임 (책임 분리)
		if (SpawnModule)
		{
			int32 SpawnCount = SpawnModule->CalculateSpawnCount(DeltaTime, SpawnFraction, bBurstFired);

			if (SpawnCount > 0)
			{
				// 파티클 간 균등한 시간 간격 계산
				float Increment = (SpawnCount > 1) ? (DeltaTime / SpawnCount) : 0.0f;
				SpawnParticles(SpawnCount, 0.0f, Increment, FVector(0.0f, 0.0f, 0.0f), FVector(0.0f, 0.0f, 0.0f));
			}
		}
	}
}

void FParticleEmitterInstance::SpawnParticles(int32 Count, float StartTime, float Increment, const FVector& InitialLocation, const FVector& InitialVelocity)
{
	if (!CurrentLODLevel)
	{
		return;
	}

	for (int32 i = 0; i < Count; i++)
	{
		// 공간이 있는지 확인
		if (ActiveParticles >= MaxActiveParticles)
		{
			// 더 많은 파티클을 수용하도록 크기 조정
			Resize(MaxActiveParticles * 2);
		}

		// 새 파티클의 인덱스 가져오기
		int32 ParticleIndex = ActiveParticles;
		ActiveParticles++;

		// 파티클 포인터 가져오기 (언리얼 방식)
		uint8* ParticleBase = ParticleData + (ParticleIndex * ParticleStride);
		DECLARE_PARTICLE_PTR(Particle, ParticleBase);

		// 생성 전
		PreSpawn(Particle, InitialLocation, InitialVelocity);

		// 생성 모듈 적용
		for (UParticleModule* Module : CurrentLODLevel->SpawnModules)
		{
			if (Module && Module->bEnabled)
			{
				Module->Spawn(this, 0, StartTime + i * Increment, Particle);
			}
		}

		// 생성 후
		float SpawnTime = StartTime + i * Increment;
		PostSpawn(Particle, static_cast<float>(i) / Count, SpawnTime);

		ParticleCounter++;
	}
}

void FParticleEmitterInstance::PreSpawn(FBaseParticle* Particle, const FVector& InitialLocation, const FVector& InitialVelocity)
{
	if (!Particle)
	{
		return;
	}

	// 기본값으로 파티클 초기화 (언리얼 엔진 호환)
	Particle->OldLocation = InitialLocation;
	Particle->Location = InitialLocation;
	Particle->BaseVelocity = InitialVelocity;
	Particle->Velocity = InitialVelocity;
	Particle->Rotation = 0.0f;
	Particle->BaseRotationRate = 0.0f;
	Particle->RotationRate = 0.0f;
	Particle->BaseSize = FVector(1.0f, 1.0f, 1.0f);
	Particle->Size = FVector(1.0f, 1.0f, 1.0f);
	Particle->Flags = 0;
	Particle->BaseColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	Particle->Color = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);
	Particle->RelativeTime = 0.0f;
	Particle->OneOverMaxLifetime = 1.0f;  // 기본 수명 1초 (1 / 1.0f = 1.0f)
}

void FParticleEmitterInstance::PostSpawn(FBaseParticle* Particle, float InterpolationParameter, float SpawnTime)
{
	// 생성 후 추가 로직을 여기에 추가할 수 있음
}

void FParticleEmitterInstance::UpdateParticles(float DeltaTime)
{
	if (!CurrentLODLevel)
	{
		return;
	}

	// 모든 파티클의 기본 속성 업데이트 (역방향 순회)
	for (int32 i = ActiveParticles - 1; i >= 0; i--)
	{
		// 파티클 가져오기
		FBaseParticle* Particle = GetParticleAtIndex(i);
		if (!Particle)
		{
			continue;
		}

		// Freeze 상태면 스킵
		if (Particle->Flags & STATE_Particle_Freeze)
		{
			continue;
		}

		// 이전 위치 저장 (충돌 처리용)
		Particle->OldLocation = Particle->Location;

		// 수명 업데이트 (최적화: 나눗셈 대신 곱셈 사용)
		Particle->RelativeTime += DeltaTime * Particle->OneOverMaxLifetime;

		// 수명 초과 시 파티클 제거
		if (Particle->RelativeTime >= 1.0f)
		{
			KillParticle(i);
			continue;
		}

		// 위치 업데이트
		Particle->Location += Particle->Velocity * DeltaTime;

		// 회전 업데이트
		Particle->Rotation += Particle->RotationRate * DeltaTime;
	}

	// 업데이트 모듈 적용 (언리얼 엔진 방식: Context 사용)
	FModuleUpdateContext Context = { *this, 0, DeltaTime };
	for (UParticleModule* Module : CurrentLODLevel->UpdateModules)
	{
		if (Module && Module->bEnabled)
		{
			Module->Update(Context);
		}
	}
}

void FParticleEmitterInstance::KillParticle(int32 Index)
{
	if (Index < 0 || Index >= ActiveParticles)
	{
		return;
	}

	// 마지막 활성 파티클과 교체
	if (Index != ActiveParticles - 1)
	{
		uint16 Temp = ParticleIndices[Index];
		ParticleIndices[Index] = ParticleIndices[ActiveParticles - 1];
		ParticleIndices[ActiveParticles - 1] = Temp;
	}

	ActiveParticles--;
}

void FParticleEmitterInstance::KillAllParticles()
{
	ActiveParticles = 0;
}

FBaseParticle* FParticleEmitterInstance::GetParticleAtIndex(int32 Index)
{
	if (Index < 0 || Index >= ActiveParticles)
	{
		return nullptr;
	}

	int32 ParticleIndex = ParticleIndices[Index];
	uint8* ParticleBase = ParticleData + (ParticleIndex * ParticleStride);
	return (FBaseParticle*)ParticleBase;
}

FDynamicEmitterDataBase* FParticleEmitterInstance::GetDynamicData(bool bSelected)
{
	// 활성 파티클이 없으면 nullptr 반환
	if (ActiveParticles <= 0)
	{
		return nullptr;
	}

	// 스프라이트 이미터 데이터 생성
	FDynamicSpriteEmitterData* NewData = new FDynamicSpriteEmitterData();

	// 소스 데이터 설정
	NewData->Source.ActiveParticleCount = ActiveParticles;
	NewData->Source.ParticleStride = ParticleStride;

	// 파티클 데이터 복사 (언리얼 엔진 방식: Alloc 사용)
	int32 ParticleDataBytes = ActiveParticles * ParticleStride;
	NewData->Source.DataContainer.Alloc(ParticleDataBytes, ActiveParticles);

	// 메모리 복사
	memcpy(NewData->Source.DataContainer.ParticleData, ParticleData, ParticleDataBytes);
	memcpy(NewData->Source.DataContainer.ParticleIndices, ParticleIndices, ActiveParticles * sizeof(uint16));

	// 언리얼 엔진 호환: Required 모듈과 Material 설정 (렌더링 시 필요)
	if (CurrentLODLevel && CurrentLODLevel->RequiredModule)
	{
		// 렌더 스레드용 데이터로 변환하여 저장
		FParticleRequiredModule* RenderData = new FParticleRequiredModule();
		*RenderData = CurrentLODLevel->RequiredModule->ToRenderThreadData();
		NewData->Source.RequiredModule = RenderData;
		NewData->Source.MaterialInterface = RenderData->Material;
	}
	else
	{
		NewData->Source.RequiredModule = nullptr;
		NewData->Source.MaterialInterface = nullptr;
	}

	// 컴포넌트 스케일 설정
	if (Component)
	{
		NewData->Source.Scale = FVector(1.0f, 1.0f, 1.0f); // 임시로 기본값 사용
	}

	return NewData;
}
