#pragma once

UENUM()
enum class ECollisionEnabled : uint8
{
    /** 물리 엔진에 어떠한 표현도 생성하지 않는다. */
    NoCollision,
    /** 공간 쿼리(레이캐스트, 스윕, 오버랩)에만 사용된다. */
    QueryOnly,
    /** 물리 시뮬레이션(강체, 제약)에만 사용된다. */
    PhysicsOnly,
    /** 공간 쿼리와 시뮬레이션 둘 다에 사용할 수 있다. */
    QueryAndPhysics
};
