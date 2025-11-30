#pragma once

#include "SphereElem.h"
#include "BoxElem.h"
#include "SphylElem.h"
#include "FKAggregateGeom.generated.h"

USTRUCT()
struct FKAggregateGeom
{
    GENERATED_REFLECTION_BODY()

    /** 구 형태 충돌체 배열 */
    UPROPERTY()
    TArray<FKSphereElem> SphereElems;

    /** 박스 형태 충돌체 배열 */
    UPROPERTY()
    TArray<FKBoxElem> BoxElems;

    /** 캡슐 형태 충돌체 배열 */
    UPROPERTY()
    TArray<FKSphylElem> SphylElems;

    FKAggregateGeom()
    {
    }

    FKAggregateGeom(const FKAggregateGeom& Other)
    {
        CloneAgg(Other);
    }

    const FKAggregateGeom& operator=(const FKAggregateGeom& Other)
    {
        CloneAgg(Other);
        return *this;
    }

    /** 전체 요소 개수 반환 */
    int32 GetElementCount() const
    {
        return SphereElems.Num() + BoxElems.Num() + SphylElems.Num();
    }

    /** 특정 타입의 요소 개수 반환 */
    int32 GetElementCount(EAggCollisionShape Type) const
    {
        switch (Type)
        {
        case EAggCollisionShape::Sphere:
            return SphereElems.Num();
        case EAggCollisionShape::Box:
            return BoxElems.Num();
        case EAggCollisionShape::Sphyl:
            return SphylElems.Num();
        default:
            return 0;
        }
    }

    /** 모든 요소 제거 */
    void EmptyElements()
    {
        SphereElems.Empty();
        BoxElems.Empty();
        SphylElems.Empty();
    }

private:
    void CloneAgg(const FKAggregateGeom& Other)
    {
        SphereElems = Other.SphereElems;
        BoxElems = Other.BoxElems;
        SphylElems = Other.SphylElems;
    }
};
