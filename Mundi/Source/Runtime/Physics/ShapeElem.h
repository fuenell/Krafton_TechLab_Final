#pragma once
#include "UEContainer.h"
#include "Vector.h"
#include "Color.h"
#include "Name.h"
#include "FKShapeElem.generated.h"

// 전방 선언
class FPrimitiveDrawInterface;

// Shape 타입 열거형
UENUM()
enum class EAggCollisionShape : uint8
{
    Sphere,
    Box,
    Sphyl,      // Capsule
    Convex,
    Unknown
};

// 충돌 활성화 타입 (나중에 CollisionTypes.h로 이동 가능)
UENUM()
enum class ECollisionEnabled : uint8
{
    NoCollision,        // 충돌 없음
    QueryOnly,          // 쿼리만 (Raycast, Overlap)
    PhysicsOnly,        // 물리만 (시뮬레이션)
    QueryAndPhysics     // 둘 다
};

/** 모든 충돌 Shape의 기본 클래스 */
USTRUCT(DisplayName="Shape Element", Description="충돌 Shape 기본 클래스")
struct FKShapeElem
{
    GENERATED_REFLECTION_BODY()

public:
    FKShapeElem()
        : ShapeType(EAggCollisionShape::Unknown)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
        , RestOffset(0.0f)
    {
    }

    explicit FKShapeElem(EAggCollisionShape InShapeType)
        : ShapeType(InShapeType)
        , bContributeToMass(true)
        , CollisionEnabled(ECollisionEnabled::QueryAndPhysics)
        , RestOffset(0.0f)
    {
    }

    FKShapeElem(const FKShapeElem& Other)
        : Name(Other.Name)
        , ShapeType(Other.ShapeType)
        , bContributeToMass(Other.bContributeToMass)
        , CollisionEnabled(Other.CollisionEnabled)
        , RestOffset(Other.RestOffset)
    {
    }

    virtual ~FKShapeElem() = default;

    FKShapeElem& operator=(const FKShapeElem& Other)
    {
        if (this != &Other)
        {
            Name = Other.Name;
            ShapeType = Other.ShapeType;
            bContributeToMass = Other.bContributeToMass;
            CollisionEnabled = Other.CollisionEnabled;
            RestOffset = Other.RestOffset;
        }
        return *this;
    }

    // Getters/Setters
    const FName& GetName() const { return Name; }
    void SetName(const FName& InName) { Name = InName; }

    EAggCollisionShape GetShapeType() const { return ShapeType; }

    bool GetContributeToMass() const { return bContributeToMass; }
    void SetContributeToMass(bool bInContributeToMass) { bContributeToMass = bInContributeToMass; }

    ECollisionEnabled GetCollisionEnabled() const { return CollisionEnabled; }
    void SetCollisionEnabled(ECollisionEnabled InCollisionEnabled) { CollisionEnabled = InCollisionEnabled; }

    // 가상 함수 (자식 클래스에서 구현)
    virtual FTransform GetTransform() const { return FTransform(); }

    // 디버그 렌더링 (자식 클래스에서 구현)
    virtual void DrawElemWire(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM,
                              float Scale, const FLinearColor& Color) const {}
    virtual void DrawElemSolid(FPrimitiveDrawInterface* PDI, const FTransform& ElemTM,
                               float Scale, const FLinearColor& Color) const {}

    // 템플릿 캐스팅
    template <typename T>
    T* GetShapeCheck()
    {
        assert(T::StaticShapeType == ShapeType);
        return static_cast<T*>(this);
    }

    template <typename T>
    const T* GetShapeCheck() const
    {
        assert(T::StaticShapeType == ShapeType);
        return static_cast<const T*>(this);
    }

public:
    // 접촉점 생성 시 오프셋 (Minkowski sum 스무딩)
    // 여유 공간을 조정해서 물리 안정성을 높임
    UPROPERTY(EditAnywhere, Category="Physics")
    float RestOffset;

protected:
    UPROPERTY(EditAnywhere, Category="Identity")
    FName Name;                                     // 사용자 정의 이름

    UPROPERTY(VisibleAnywhere, Category="Physics")
    EAggCollisionShape ShapeType;                   // Shape 타입

    UPROPERTY(EditAnywhere, Category="Physics")
    bool bContributeToMass;                         // 질량 기여 여부

    UPROPERTY(EditAnywhere, Category="Collision")
    ECollisionEnabled CollisionEnabled;             // 충돌 활성화 상태
};
