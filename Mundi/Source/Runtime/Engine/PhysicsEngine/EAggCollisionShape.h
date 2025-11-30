#pragma once

UENUM()
enum class EAggCollisionShape : uint8
{
    Sphere,
    Box,
    Sphyl,
    Convex,
    TaperedCapsule,
    LevelSet,
    Unknown
};
