#pragma once

#include "Source/Runtime/Core/Math/Vector.h"
#include "EPhysicsShapeType.h"
#include "EConstraintType.h"

/**
 * IPhysicsAssetPreview
 *
 * PhysX 팀 연동을 위한 인터페이스
 * 에디터에서 시뮬레이션 미리보기 기능 제공
 * (현재는 더미 구현, 추후 PhysX 팀이 구현)
 */
class IPhysicsAssetPreview
{
public:
	virtual ~IPhysicsAssetPreview() = default;

	// 시뮬레이션 시작/중지
	virtual void StartSimulation() = 0;
	virtual void StopSimulation() = 0;
	virtual bool IsSimulating() const = 0;

	// 바디 위치 업데이트 (시뮬레이션 결과 적용)
	virtual void UpdateBodyTransforms(const TArray<FTransform>& Transforms) = 0;
};
