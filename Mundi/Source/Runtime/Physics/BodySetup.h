#pragma once
#include "Object.h"
#include "AggregateGeom.h"
#include "PhysXConversion.h"
#include "PhysicalMaterial.h"
#include "UBodySetup.generated.h"

// 전방 선언
struct FBodyInstance;

/**
 * UBodySetup
 *
 * 물리 바디의 충돌 형태를 정의하는 기본 클래스.
 * StaticMesh 등의 단순 충돌 Shape 관리에 사용.
 *
 * Unreal Engine의 UBodySetup과 유사한 역할:
 * - FKAggregateGeom을 통해 복합 충돌 Shape 관리
 * - PhysX Shape 생성 팩토리 기능 제공
 *
 * SkeletalMesh용은 USkeletalBodySetup을 사용할 것.
 */
UCLASS(DisplayName="바디 셋업", Description="물리 바디의 충돌 형태 정의")
class UBodySetup : public UObject
{
    GENERATED_REFLECTION_BODY()

public:
    // --- 본 정보 ---

    // 이 BodySetup이 연결된 본 이름 (스켈레탈 메시용)
    UPROPERTY(EditAnywhere, Category="Bone")
    FName BoneName = "None";

    // --- 기본 설정 ---

    // 충돌 Geometry 컨테이너 (Sphere, Box, Capsule 등)
    FKAggregateGeom AggGeom;

    // 기본 충돌 활성화 상태
    UPROPERTY(EditAnywhere, Category="Physics")
    ECollisionEnabled DefaultCollisionEnabled = ECollisionEnabled::QueryAndPhysics;

    // 물리 재질 (nullptr이면 기본 재질 사용)
    UPhysicalMaterial* PhysMaterial = nullptr;

    // --- 생성자/소멸자 ---
    UBodySetup();
    virtual ~UBodySetup();

    // --- PhysX Shape 생성 ---

    /**
     * AggGeom의 모든 Shape를 PhysX Shape로 변환하여 BodyInstance에 추가
     *
     * @param BodyInstance 대상 FBodyInstance
     * @param Scale3D 적용할 스케일
     * @param InMaterial 사용할 물리 재질 (nullptr이면 PhysMaterial 또는 기본 재질 사용)
     */
    void CreatePhysicsShapes(FBodyInstance* BodyInstance, const FVector& Scale3D = FVector::One(),
                             UPhysicalMaterial* InMaterial = nullptr);

    /**
     * 개별 Shape 생성 함수들 (내부 사용)
     */
    physx::PxShape* CreateSphereShape(const FKSphereElem& Elem, const FVector& Scale3D,
                                       physx::PxMaterial* Material);
    physx::PxShape* CreateBoxShape(const FKBoxElem& Elem, const FVector& Scale3D,
                                    physx::PxMaterial* Material);
    physx::PxShape* CreateCapsuleShape(const FKSphylElem& Elem, const FVector& Scale3D,
                                        physx::PxMaterial* Material);

    // --- 유틸리티 ---

    // 전체 볼륨 계산
    float GetVolume(const FVector& Scale3D = FVector::One()) const;

    // Shape가 있는지 확인
    bool HasValidShapes() const { return !AggGeom.IsEmpty(); }

    // 전체 Shape 개수
    int32 GetShapeCount() const { return AggGeom.GetElementCount(); }

    // --- Shape 추가 헬퍼 함수 ---

    // Sphere 추가
    int32 AddSphereElem(float Radius, const FVector& Center = FVector::Zero());

    // Box 추가
    int32 AddBoxElem(float HalfX, float HalfY, float HalfZ,
                     const FVector& Center = FVector::Zero(),
                     const FQuat& Rotation = FQuat::Identity());

    // Capsule 추가 (Z축이 캡슐 축)
    int32 AddCapsuleElem(float Radius, float HalfHeight,
                         const FVector& Center = FVector::Zero(),
                         const FQuat& Rotation = FQuat::Identity());

    // 모든 Shape 제거
    void ClearAllShapes();

    // --- 직렬화 ---
    virtual void Serialize(const bool bInIsLoading, JSON& InOutHandle) override;
};
