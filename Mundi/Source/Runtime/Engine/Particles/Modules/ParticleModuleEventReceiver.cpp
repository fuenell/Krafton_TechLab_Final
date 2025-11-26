#include "pch.h"
#include "ParticleModuleEventReceiver.h"
#include "ParticleEmitterInstance.h"
#include "ParticleSystemComponent.h"

// ============================================================================
// UParticleModuleEventReceiverBase
// ============================================================================

void UParticleModuleEventReceiverBase::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModule::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		int32 TypeValue = 0;
		FJsonSerializer::ReadInt32(InOutHandle, "EventType", TypeValue);
		EventType = static_cast<EParticleEventType>(TypeValue);
		FJsonSerializer::ReadString(InOutHandle, "EventName", EventName);
	}
	else
	{
		InOutHandle["EventType"] = static_cast<int32>(EventType);
		InOutHandle["EventName"] = EventName;
	}
}

// ============================================================================
// UParticleModuleEventReceiverSpawn
// ============================================================================

void UParticleModuleEventReceiverSpawn::HandleEvent(FParticleEmitterInstance* Owner, const FParticleEventData& Event)
{
	if (!Owner)
	{
		return;
	}

	// 이벤트 타입 필터링
	if (EventType != EParticleEventType::Any && EventType != Event.Type)
	{
		return;
	}

	// 분포에서 값 샘플링
	float ActualVelocityScale = VelocityScale.GetValue(Owner->EmitterTime, Owner->RandomStream, Owner->Component);

	// 위치 및 속도 결정
	FVector SpawnLocation = bUseEventLocation ? Event.Position : Owner->CachedEmitterOrigin;
	FVector SpawnVelocity = bInheritVelocity ? Event.Velocity * ActualVelocityScale : FVector(0.0f, 0.0f, 0.0f);

	SpawnParticlesAtLocation(Owner, SpawnLocation, SpawnVelocity);
}

void UParticleModuleEventReceiverSpawn::HandleCollisionEvent(FParticleEmitterInstance* Owner, const FParticleEventCollideData& Event)
{
	if (!Owner)
	{
		return;
	}

	// 충돌 이벤트 타입 필터링
	if (EventType != EParticleEventType::Any && EventType != EParticleEventType::Collision)
	{
		return;
	}

	// 분포에서 값 샘플링
	float ActualVelocityScale = VelocityScale.GetValue(Owner->EmitterTime, Owner->RandomStream, Owner->Component);

	// 위치 및 속도 결정
	FVector SpawnLocation = bUseEventLocation ? Event.Position : Owner->CachedEmitterOrigin;
	FVector SpawnVelocity = bInheritVelocity ? Event.Velocity * ActualVelocityScale : FVector(0.0f, 0.0f, 0.0f);

	// 충돌 이벤트의 경우 법선 방향으로 속도 조정 가능
	// (충돌 후 튕겨나가는 효과)

	SpawnParticlesAtLocation(Owner, SpawnLocation, SpawnVelocity);
}

void UParticleModuleEventReceiverSpawn::SpawnParticlesAtLocation(FParticleEmitterInstance* Owner, const FVector& Location, const FVector& Velocity)
{
	if (!Owner || !Owner->Component)
	{
		return;
	}

	// Component 파괴 상태 체크 (언리얼 방식)
	if (Owner->Component->IsPendingDestroy())
	{
		return;
	}

	// 분포에서 스폰 수 샘플링
	float SpawnCountFloat = SpawnCount.GetValue(Owner->EmitterTime, Owner->RandomStream, Owner->Component);
	int32 ActualSpawnCount = FMath::Max(1, static_cast<int32>(SpawnCountFloat + 0.5f));

	// 파티클 스폰 (스폰 수 제한은 Resize 하드 리밋에서 처리됨)
	// StartTime = 0 (현재 프레임에서 즉시 스폰)
	// Increment = 0 (모든 파티클이 같은 시간에 스폰)
	Owner->SpawnParticles(ActualSpawnCount, 0.0f, 0.0f, Location, Velocity);
}

void UParticleModuleEventReceiverSpawn::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModuleEventReceiverBase::Serialize(bInIsLoading, InOutHandle);

	JSON TempJson;
	if (bInIsLoading)
	{
		if (FJsonSerializer::ReadObject(InOutHandle, "SpawnCount", TempJson))
			SpawnCount.Serialize(true, TempJson);
		FJsonSerializer::ReadBool(InOutHandle, "bUseEventLocation", bUseEventLocation);
		FJsonSerializer::ReadBool(InOutHandle, "bInheritVelocity", bInheritVelocity);
		if (FJsonSerializer::ReadObject(InOutHandle, "VelocityScale", TempJson))
			VelocityScale.Serialize(true, TempJson);
	}
	else
	{
		TempJson = JSON::Make(JSON::Class::Object);
		SpawnCount.Serialize(false, TempJson);
		InOutHandle["SpawnCount"] = TempJson;

		InOutHandle["bUseEventLocation"] = bUseEventLocation;
		InOutHandle["bInheritVelocity"] = bInheritVelocity;

		TempJson = JSON::Make(JSON::Class::Object);
		VelocityScale.Serialize(false, TempJson);
		InOutHandle["VelocityScale"] = TempJson;
	}
}

// ============================================================================
// UParticleModuleEventReceiverKill
// ============================================================================

void UParticleModuleEventReceiverKill::HandleEvent(FParticleEmitterInstance* Owner, const FParticleEventData& Event)
{
	if (!Owner)
	{
		return;
	}

	// 이벤트 타입 필터링
	if (EventType != EParticleEventType::Any && EventType != Event.Type)
	{
		return;
	}

	KillParticles(Owner, Event.Position);
}

void UParticleModuleEventReceiverKill::KillParticles(FParticleEmitterInstance* Owner, const FVector& EventLocation)
{
	if (!Owner)
	{
		return;
	}

	int32 ParticlesToKill = (KillCount == 0) ? Owner->ActiveParticles : FMath::Min(KillCount, Owner->ActiveParticles);

	if (bKillNearestFirst && ParticlesToKill < Owner->ActiveParticles)
	{
		// 거리 기반으로 정렬하여 가장 가까운 파티클부터 제거
		// 간단한 구현: 거리 배열 생성 후 정렬
		TArray<TPair<int32, float>> DistanceArray;
		DistanceArray.Reserve(Owner->ActiveParticles);

		for (int32 i = 0; i < Owner->ActiveParticles; ++i)
		{
			FBaseParticle* Particle = Owner->GetParticleAtIndex(i);
			if (Particle)
			{
				FVector Diff = Particle->Location - EventLocation;
				float DistSq = FVector::Dot(Diff, Diff);
				DistanceArray.Add(TPair<int32, float>(i, DistSq));
			}
		}

		// 거리로 정렬 (가까운 것 먼저)
		DistanceArray.Sort([](const TPair<int32, float>& A, const TPair<int32, float>& B) {
			return A.second < B.second;
		});

		// 가장 가까운 파티클부터 제거 (역순으로 제거해야 인덱스 유효)
		TArray<int32> IndicesToKill;
		for (int32 i = 0; i < ParticlesToKill && i < DistanceArray.Num(); ++i)
		{
			IndicesToKill.Add(DistanceArray[i].first);
		}

		// 인덱스 역순 정렬 (큰 인덱스부터 제거)
		IndicesToKill.Sort([](const int32& A, const int32& B) {
			return A > B;
		});

		for (int32 Index : IndicesToKill)
		{
			Owner->KillParticle(Index);
		}
	}
	else
	{
		// 단순히 앞에서부터 제거
		for (int32 i = ParticlesToKill - 1; i >= 0; --i)
		{
			Owner->KillParticle(i);
		}
	}
}

void UParticleModuleEventReceiverKill::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
	UParticleModuleEventReceiverBase::Serialize(bInIsLoading, InOutHandle);

	if (bInIsLoading)
	{
		FJsonSerializer::ReadInt32(InOutHandle, "KillCount", KillCount);
		FJsonSerializer::ReadBool(InOutHandle, "bKillNearestFirst", bKillNearestFirst);
	}
	else
	{
		InOutHandle["KillCount"] = KillCount;
		InOutHandle["bKillNearestFirst"] = bKillNearestFirst;
	}
}
