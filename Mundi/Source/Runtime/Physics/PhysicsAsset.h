#pragma once
#include "Object.h"
#include "SkeletalBodySetup.h"
#include "UPhysicsAsset.generated.h"

// 전방 선언
class USkeletalMesh;

/**
 * UPhysicsAsset
 *
 * SkeletalMesh의 물리 시뮬레이션을 위한 BodySetup 컨테이너.
 * 각 본(Bone)에 대응하는 충돌 Shape와 물리 속성을 정의.
 *
 * 주요 용도:
 * - 랙돌(Ragdoll) 물리
 * - 물리 기반 애니메이션
 * - 캐릭터 충돌 처리
 */
UCLASS(DisplayName="Physics Asset", Description="스켈레탈 메시용 물리 에셋")
class UPhysicsAsset : public UObject
{
    GENERATED_REFLECTION_BODY()

public:
    // --- BodySetup 컬렉션 ---

    // 모든 SkeletalBodySetup 배열 (각 본에 대응)
    UPROPERTY(EditAnywhere, Category="Physics")
    TArray<USkeletalBodySetup*> SkeletalBodySetups;

    // --- 기본 물리 설정 ---

    // 기본 물리 재질 (개별 BodySetup에서 오버라이드 가능)
    UPROPERTY(EditAnywhere, Category="Physics")
    UPhysicalMaterial* DefaultPhysMaterial = nullptr;

    // 전체 시뮬레이션 활성화 여부
    UPROPERTY(EditAnywhere, Category="Physics")
    bool bEnablePhysicsSimulation = true;

    // --- 생성자/소멸자 ---
    UPhysicsAsset();
    virtual ~UPhysicsAsset();

    // --- BodySetup 관리 ---

    // 본 이름으로 BodySetup 검색
    USkeletalBodySetup* FindBodySetup(const FName& BoneName) const;

    // 본 인덱스로 BodySetup 검색
    USkeletalBodySetup* GetBodySetup(int32 BodyIndex) const;

    // BodySetup 추가
    int32 AddBodySetup(USkeletalBodySetup* InBodySetup);

    // 본 이름으로 BodySetup 인덱스 검색
    int32 FindBodySetupIndex(const FName& BoneName) const;

    // BodySetup 개수
    int32 GetBodySetupCount() const { return SkeletalBodySetups.Num(); }

    // BodySetup 제거
    void RemoveBodySetup(int32 Index);
    void RemoveBodySetup(const FName& BoneName);

    // 모든 BodySetup 제거
    void ClearAllBodySetups();

    // --- 유틸리티 ---

    // 유효성 검사
    bool IsValid() const { return SkeletalBodySetups.Num() > 0; }

    // 전체 Shape 개수 계산
    int32 GetTotalShapeCount() const;

    // --- 직렬화 ---
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
