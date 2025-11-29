#pragma once
#include "Object.h"
#include "UEContainer.h"
#include "Actor.h"
#include <algorithm>
#include "ULevel.generated.h"

UCLASS()
class ULevel : public UObject
{
public:
    GENERATED_REFLECTION_BODY()
    ULevel() = default;
    ~ULevel() override = default;

    const TArray<AActor*>& GetActors() const { return Actors; }
    void AddActor(AActor* Actor) { if (Actor) Actors.Add(Actor); }
    void SpawnDefaultActors();
    bool RemoveActor(AActor* Actor)
    {
        auto it = std::find(Actors.begin(), Actors.end(), Actor);
        if (it != Actors.end()) { Actors.erase(it); return true; }
        return false;
    }
    void Clear() { Actors.Empty(); }

    void Serialize(const bool bInIsLoading, JSON& InOutHandle);

    // ───── 가비지 컬렉션 관련 ────────────────────────────
    void AddReferencedObjects(FGarbageCollector& Collector) override;
private:
    TArray<AActor*> Actors;
};

class ULevelService
{
public:
    // Create a new empty level
    static std::unique_ptr<ULevel> CreateNewLevel();
    static std::unique_ptr<ULevel> CreateDefaultLevel();
};
