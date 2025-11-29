#include "pch.h"
#include "PhysicsAsset.h"

// --- 생성자/소멸자 ---

UPhysicsAsset::UPhysicsAsset()
    : DefaultPhysMaterial(nullptr)
    , bEnablePhysicsSimulation(true)
{
}

UPhysicsAsset::~UPhysicsAsset()
{
    ClearAllBodySetups();
}

// --- BodySetup 관리 ---

UBodySetup* UPhysicsAsset::FindBodySetup(const FName& BoneName) const
{
    for (int32 i = 0; i < SkeletalBodySetups.Num(); ++i)
    {
        UBodySetup* Setup = SkeletalBodySetups[i];
        if (Setup && Setup->BoneName == BoneName)
        {
            return Setup;
        }
    }
    return nullptr;
}

UBodySetup* UPhysicsAsset::GetBodySetup(int32 BodyIndex) const
{
    if (BodyIndex >= 0 && BodyIndex < SkeletalBodySetups.Num())
    {
        return SkeletalBodySetups[BodyIndex];
    }
    return nullptr;
}

int32 UPhysicsAsset::AddBodySetup(UBodySetup* InBodySetup)
{
    if (!InBodySetup) return -1;

    // 중복 체크
    int32 ExistingIndex = FindBodySetupIndex(InBodySetup->BoneName);
    if (ExistingIndex != -1)
    {
        // 이미 존재하면 교체
        SkeletalBodySetups[ExistingIndex] = InBodySetup;
        return ExistingIndex;
    }

    return SkeletalBodySetups.Add(InBodySetup);
}

int32 UPhysicsAsset::FindBodySetupIndex(const FName& BoneName) const
{
    for (int32 i = 0; i < SkeletalBodySetups.Num(); ++i)
    {
        UBodySetup* Setup = SkeletalBodySetups[i];
        if (Setup && Setup->BoneName == BoneName)
        {
            return i;
        }
    }
    return -1;
}

void UPhysicsAsset::RemoveBodySetup(int32 Index)
{
    if (Index >= 0 && Index < SkeletalBodySetups.Num())
    {
        SkeletalBodySetups.RemoveAt(Index);
    }
}

void UPhysicsAsset::RemoveBodySetup(const FName& BoneName)
{
    int32 Index = FindBodySetupIndex(BoneName);
    if (Index != -1)
    {
        RemoveBodySetup(Index);
    }
}

void UPhysicsAsset::ClearAllBodySetups()
{
    SkeletalBodySetups.Empty();
}

// --- 유틸리티 ---

int32 UPhysicsAsset::GetTotalShapeCount() const
{
    int32 TotalCount = 0;
    for (int32 i = 0; i < SkeletalBodySetups.Num(); ++i)
    {
        UBodySetup* Setup = SkeletalBodySetups[i];
        if (Setup)
        {
            TotalCount += Setup->GetShapeCount();
        }
    }
    return TotalCount;
}

// --- 직렬화 ---

void UPhysicsAsset::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // TODO: SkeletalBodySetups 배열 직렬화
    // TODO: DefaultPhysMaterial 참조 직렬화
}
