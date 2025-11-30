#pragma once

/**
 * EConstraintType
 *
 * 물리 제약 조건 타입
 * UE의 11개 타입에서 핵심 2개로 단순화
 */
UENUM()
enum class EConstraintType : uint8
{
	BallAndSocket,  // 구형 조인트 (3축 회전)
	Hinge,          // 힌지 조인트 (1축 회전)
};

/** Constraint 타입 이름 반환 */
inline const char* GetConstraintTypeName(EConstraintType Type)
{
	switch (Type)
	{
	case EConstraintType::BallAndSocket: return "Ball & Socket";
	case EConstraintType::Hinge: return "Hinge";
	default: return "Unknown";
	}
}
