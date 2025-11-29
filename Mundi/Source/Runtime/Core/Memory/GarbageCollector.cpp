#include "pch.h"
#include "GarbageCollector.h"
#include "Object.h"
#include "ObjectFactory.h"

FGarbageCollector& FGarbageCollector::Get()
{
    static FGarbageCollector Instance;
    return Instance;
}

void FGarbageCollector::AddRoot(UObject* Object)
{
    if (Object)
    {
        RootObjects.insert(Object);
    }
}

void FGarbageCollector::RemoveRoot(UObject* Object)
{
    if (Object)
    {
        RootObjects.erase(Object);
    }
}

bool FGarbageCollector::IsRoot(UObject* Object) const
{
    return Object && RootObjects.find(Object) != RootObjects.end();
}

void FGarbageCollector::CollectGarbage()
{
    if (!bIsEnabled || bIsCollecting)
    {
        return;
    }

    bIsCollecting = true;

    // 1. 모든 객체의 마크 플래그 초기화
    ResetMarks();

    // 2. Mark 단계: Root부터 도달 가능한 객체들 마킹
     MarkPhase();

    // 3. Sweep 단계: 마킹되지 않은 객체 삭제
    SweepPhase();

    bIsCollecting = false;
}

void FGarbageCollector::MarkObject(UObject* Object)
{
    if (!Object)
    {
        return;
    }

    // 이미 마킹된 객체는 스킵
    if (Object->IsMarkedByGC())
    {
        return;
    }

    // 마킹하고 큐에 추가
    Object->MarkByGC();
    MarkQueue.Add(Object);
}

void FGarbageCollector::ResetMarks()
{
    for (UObject* Obj : GUObjectArray)
    {
        if (Obj)
        {
            Obj->UnmarkByGC();
        }
    }
}

void FGarbageCollector::MarkPhase()
{
    MarkQueue.Empty();

    // Root 객체들을 마킹 큐에 추가
    for (UObject* Root : RootObjects)
    {
        if (Root)
        {
            MarkObject(Root);
        }
    }

    // BFS 방식으로 참조 그래프 순회
    while (MarkQueue.Num() > 0)
    {
        // 큐에서 객체 하나 꺼내기
        UObject* Current = MarkQueue[0];
        MarkQueue.RemoveAt(0);

        if (Current)
        {
            // 이 객체가 참조하는 다른 객체들을 마킹
            Current->AddReferencedObjects(*this);
        }
    }
}

void FGarbageCollector::SweepPhase()
{
    LastCollectedCount = 0;
    TArray<UObject*> ObjectsToDelete;

    // 마킹되지 않은 객체들 수집
    for (int32 i = 0; i < GUObjectArray.Num(); ++i)
    {
        UObject* Obj = GUObjectArray[i];
        if (Obj && !Obj->IsMarkedByGC())
        {
            ObjectsToDelete.Add(Obj);
        }
    }

    // 수집된 객체들 삭제
    for (UObject* Obj : ObjectsToDelete)
    {
        ObjectFactory::DeleteObject(Obj);
        LastCollectedCount++;
    }

    TotalCollectedCount += LastCollectedCount;

    // Null 슬롯 압축 (선택적)
    if (LastCollectedCount > 0)
    {
        ObjectFactory::CompactNullSlots();
    }
}
