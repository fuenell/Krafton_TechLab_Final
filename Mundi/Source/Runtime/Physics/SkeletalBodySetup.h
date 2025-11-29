#pragma once
#include "BodySetup.h"
#include "USkeletalBodySetup.generated.h"

/**
 * USkeletalBodySetup
 *
 * SkeletalMesh의 본(Bone)에 연결되는 물리 바디 설정.
 * UBodySetup을 상속받아 스켈레탈 메시 전용 기능을 추가.
 *
 * 현재는 UBodySetup과 동일하며, 추후 Physical Animation Profile 등
 * 스켈레탈 전용 기능이 필요할 때 확장.
 */
UCLASS(DisplayName="스켈레탈 바디 셋업", Description="스켈레탈 메시용 물리 바디 설정")
class USkeletalBodySetup : public UBodySetup
{
    GENERATED_REFLECTION_BODY()

public:
    // --- 생성자/소멸자 ---
    USkeletalBodySetup();
    virtual ~USkeletalBodySetup();

    // --- 직렬화 ---
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
