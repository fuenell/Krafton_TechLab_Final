#pragma once

#include "EAggCollisionShape.h"
#include "ECollisionEnabled.h"
#include "FKShapeElem.generated.h"

USTRUCT()
struct FKShapeElem
{
    GENERATED_REFLECTION_BODY()

    FKShapeElem()
        : ShapeType(EAggCollisionShape::Unknown)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
    {
    }

    FKShapeElem(EAggCollisionShape InShapeType)
        : ShapeType(InShapeType)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
    {
    }

    FKShapeElem(const FKShapeElem& Other)
        : ShapeType(Other.ShapeType)
        , Name(Other.Name)
        , bContributeToMass(Other.bContributeToMass)
        , CollisionEnabled(Other.CollisionEnabled)
    {
    }

    virtual ~FKShapeElem() = default;

    static inline EAggCollisionShape StaticShapeType = EAggCollisionShape::Unknown;

    const FName& GetName() const { return Name; }
    void SetName(const FName& InName) { Name = InName; }
    EAggCollisionShape GetShapeType() const { return ShapeType; }
    bool GetContributeToMass() const { return bContributeToMass; }
    void SetContributeToMass(bool bInContributeToMass) { bContributeToMass = bInContributeToMass; }
    ECollisionEnabled GetCollisionEnabled() const { return CollisionEnabled; }
    void SetCollisionEnabled(ECollisionEnabled InCollisionEnabled) { CollisionEnabled = InCollisionEnabled; }
    virtual FTransform GetTransform() const { return FTransform(); }

    /** 충돌체 타입 */
    UPROPERTY()
    EAggCollisionShape ShapeType;

    /** 식별용 이름 */
    UPROPERTY()
    FName Name;

    /** 쉐이프가 강체의 질량 계산에 기여할지 여부 */
    UPROPERTY()
    bool bContributeToMass;

    /** 충돌 활성화 상태 */
    UPROPERTY()
    ECollisionEnabled CollisionEnabled;
};
