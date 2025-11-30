#pragma once

/**
 * EPhysicsShapeType
 *
 * Physics Body의 충돌 형태 타입
 * UE의 EPhysicsShapeType을 단순화
 */
UENUM()
enum class EPhysicsShapeType : uint8
{
	Sphere,
	Capsule,
	Box,
};

/** Shape 타입 이름 반환 */
inline const char* GetShapeTypeName(EPhysicsShapeType Type)
{
	switch (Type)
	{
	case EPhysicsShapeType::Sphere: return "Sphere";
	case EPhysicsShapeType::Capsule: return "Capsule";
	case EPhysicsShapeType::Box: return "Box";
	default: return "Unknown";
	}
}
