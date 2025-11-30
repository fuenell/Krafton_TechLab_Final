#pragma once
#include "ShapeElem.h"
#include "PhysXSupport.h"
#include "FKSphereElem.generated.h"

/** 충돌을 위해 사용되는 구 도형 */
USTRUCT()
struct FKSphereElem : public FKShapeElem
{
    GENERATED_REFLECTION_BODY()

    static inline EAggCollisionShape StaticShapeType = EAggCollisionShape::Sphere;

    /** 구의 중심 */
    UPROPERTY()
    FVector Center;

    /** 구의 반지름 */
    UPROPERTY()
    float Radius;

    FKSphereElem()
        : FKShapeElem(EAggCollisionShape::Sphere)
        , Center(FVector::Zero())
        , Radius(0.5f)
    {
    }

    FKSphereElem(float InRadius)
        : FKShapeElem(EAggCollisionShape::Sphere)
        , Center(FVector::Zero())
        , Radius(InRadius)
    {
    }

    virtual ~FKSphereElem() = default;

    virtual FTransform GetTransform() const override final
    {
        FTransform Transform;
        Transform.Translation = Center;
        Transform.Rotation = FQuat::Identity();
        return Transform;
    }

    void SetTransform(const FTransform& InTransform)
    {
        Center = InTransform.Translation;
    }

    PxSphereGeometry GetPxGeometry(const FVector& Scale3D) const
    {
        float MaxScale = FMath::Max(
            FMath::Abs(Scale3D.X),
            FMath::Max(FMath::Abs(Scale3D.Y), FMath::Abs(Scale3D.Z))
        );
        constexpr float MinRadius = 0.01f;
        return physx::PxSphereGeometry(FMath::Max(Radius * MaxScale, MinRadius));
    }
};
