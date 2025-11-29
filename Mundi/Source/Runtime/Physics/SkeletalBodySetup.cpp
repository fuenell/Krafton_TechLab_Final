#include "pch.h"
#include "SkeletalBodySetup.h"

// --- 생성자/소멸자 ---

USkeletalBodySetup::USkeletalBodySetup()
{
}

USkeletalBodySetup::~USkeletalBodySetup()
{
}

// --- 직렬화 ---

void USkeletalBodySetup::Serialize(const bool bInIsLoading, JSON& InOutHandle)
{
    Super::Serialize(bInIsLoading, InOutHandle);

    // 추후 스켈레탈 전용 데이터 직렬화 시 구현
}
