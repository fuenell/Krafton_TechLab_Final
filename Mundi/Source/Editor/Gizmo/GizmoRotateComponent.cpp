#include "pch.h"
#include "GizmoRotateComponent.h"

IMPLEMENT_CLASS(UGizmoRotateComponent)

UGizmoRotateComponent::UGizmoRotateComponent()
{
    SetStaticMesh("Data/Gizmo/RotationHandle.obj");
    SetMaterialByName(0, "Shaders/StaticMesh/Gizmo.hlsl");
}

UGizmoRotateComponent::~UGizmoRotateComponent()
{
}

void UGizmoRotateComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}
