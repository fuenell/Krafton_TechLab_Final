#pragma once
#include "ShapeElem.h"
#include "PhysXSupport.h"
#include "FKBoxElem.generated.h"

/** 충돌을 위해 사용되는 박스 도형 */
USTRUCT()
struct FKBoxElem : public FKShapeElem
{
    GENERATED_REFLECTION_BODY()

    static inline EAggCollisionShape StaticShapeType = EAggCollisionShape::Box;

    /** 박스의 원점 */
    UPROPERTY()
    FVector Center;

    /** 박스의 회전 (쿼터니언) */
    UPROPERTY()
    FQuat Rotation;

    /** x축 방향 박스의 너비 (반지름이 아닌 지름) */
    UPROPERTY()
    float X;

    /** Y축 방향 박스의 너비 (반지름이 아닌 지름) */
    UPROPERTY()
    float Y;

    /** Z축 방향 박스의 너비 (반지름이 아닌 지름) */
    UPROPERTY()
    float Z;

    FKBoxElem()
        : FKShapeElem(EAggCollisionShape::Box)
        , Center(FVector::Zero())
        , Rotation(FQuat::Identity())
        , X(1.f), Y(1.f), Z(1.f)
    {
    }

    FKBoxElem(float s)
        : FKShapeElem(EAggCollisionShape::Box)
        , Center(FVector::Zero())
        , Rotation(FQuat::Identity())
        , X(s), Y(s), Z(s)
    {
    }

    FKBoxElem(float InX, float InY, float InZ)
        : FKShapeElem(EAggCollisionShape::Box)
        , Center(FVector::Zero())
        , Rotation(FQuat::Identity())
        , X(InX), Y(InY), Z(InZ)
    {
    }

    virtual ~FKBoxElem() = default;

    virtual FTransform GetTransform() const override final
    {
        FTransform Transform;
        Transform.Translation = Center;
        Transform.Rotation = Rotation;
        return Transform;
    }

    void SetTransform(const FTransform& InTransform)
    {
        Center = InTransform.Translation;
        Rotation = InTransform.Rotation;
    }

    PxBoxGeometry GetPxGeometry(const FVector& Scale3D) const
    {
        float AbsScaleX = FMath::Abs(Scale3D.X);
        float AbsScaleY = FMath::Abs(Scale3D.Y);
        float AbsScaleZ = FMath::Abs(Scale3D.Z);

        float HalfX = (X * 0.5f) * AbsScaleX;
        float HalfY = (Y * 0.5f) * AbsScaleY;
        float HalfZ = (Z * 0.5f) * AbsScaleZ;

        constexpr float MinExtent = 0.01f;

        return physx::PxBoxGeometry(
            FMath::Max(HalfX, MinExtent),
            FMath::Max(HalfY, MinExtent),
            FMath::Max(HalfZ, MinExtent)
        );
    }
};
