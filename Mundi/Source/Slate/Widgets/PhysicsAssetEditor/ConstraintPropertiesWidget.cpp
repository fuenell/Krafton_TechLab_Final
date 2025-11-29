#include "pch.h"
#include "ConstraintPropertiesWidget.h"
#include "ImGui/imgui.h"
#include "Source/Runtime/Engine/Viewer/PhysicsAssetEditorState.h"
#include "Source/Runtime/Engine/PhysicsEngine/PhysicsAsset.h"
#include "Source/Runtime/Engine/PhysicsEngine/FConstraintSetup.h"

IMPLEMENT_CLASS(UConstraintPropertiesWidget);

UConstraintPropertiesWidget::UConstraintPropertiesWidget()
	: UWidget("ConstraintPropertiesWidget")
{
}

void UConstraintPropertiesWidget::Initialize()
{
	bWasModified = false;
}

void UConstraintPropertiesWidget::Update()
{
	// 필요시 업데이트 로직 추가
}

void UConstraintPropertiesWidget::RenderWidget()
{
	if (!EditorState || !EditorState->EditingAsset) return;

	// 선택된 제약조건이 없으면 렌더링하지 않음
	if (EditorState->bBodySelectionMode || EditorState->SelectedConstraintIndex < 0) return;
	if (EditorState->SelectedConstraintIndex >= static_cast<int32>(EditorState->EditingAsset->ConstraintSetups.size())) return;

	FConstraintSetup& Constraint = EditorState->EditingAsset->ConstraintSetups[EditorState->SelectedConstraintIndex];
	bWasModified = false;

	ImGui::Text("Constraint: %s", Constraint.JointName.ToString().c_str());
	ImGui::Separator();

	// 제약 조건 타입
	const char* ConstraintTypes[] = { "Ball and Socket", "Hinge" };
	int32 CurrentType = static_cast<int32>(Constraint.ConstraintType);
	if (ImGui::Combo("Type", &CurrentType, ConstraintTypes, IM_ARRAYSIZE(ConstraintTypes)))
	{
		Constraint.ConstraintType = static_cast<EConstraintType>(CurrentType);
		EditorState->bIsDirty = true;
		bWasModified = true;
	}

	// 각도 제한
	if (ImGui::CollapsingHeader("Limits", ImGuiTreeNodeFlags_DefaultOpen))
	{
		bWasModified |= RenderLimitProperties(Constraint);
	}

	// 강도/댐핑
	if (ImGui::CollapsingHeader("Stiffness & Damping"))
	{
		if (ImGui::DragFloat("Stiffness", &Constraint.Stiffness, 0.1f, 0.0f, 10000.0f))
		{
			EditorState->bIsDirty = true;
			bWasModified = true;
		}
		if (ImGui::DragFloat("Damping", &Constraint.Damping, 0.1f, 0.0f, 1000.0f))
		{
			EditorState->bIsDirty = true;
			bWasModified = true;
		}
	}
}

bool UConstraintPropertiesWidget::RenderLimitProperties(FConstraintSetup& Constraint)
{
	bool bChanged = false;

	if (ImGui::DragFloat("Swing1 Limit", &Constraint.Swing1Limit, 1.0f, 0.0f, 180.0f))
	{
		EditorState->bIsDirty = true;
		bChanged = true;
	}

	// BallAndSocket만 Swing2와 Twist 사용
	if (Constraint.ConstraintType == EConstraintType::BallAndSocket)
	{
		if (ImGui::DragFloat("Swing2 Limit", &Constraint.Swing2Limit, 1.0f, 0.0f, 180.0f))
		{
			EditorState->bIsDirty = true;
			bChanged = true;
		}

		if (ImGui::DragFloat("Twist Min", &Constraint.TwistLimitMin, 1.0f, -180.0f, 0.0f))
		{
			EditorState->bIsDirty = true;
			bChanged = true;
		}

		if (ImGui::DragFloat("Twist Max", &Constraint.TwistLimitMax, 1.0f, 0.0f, 180.0f))
		{
			EditorState->bIsDirty = true;
			bChanged = true;
		}
	}

	return bChanged;
}
