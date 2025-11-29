#include "pch.h"
#include "GizmoScaleComponent.h"


UGizmoScaleComponent::UGizmoScaleComponent()
{
    SetStaticMesh(GDataDir + "/Gizmo/ScaleHandle.obj");
    SetMaterialByName(0, "Shaders/UI/Gizmo.hlsl");
}

UGizmoScaleComponent::~UGizmoScaleComponent()
{
}

void UGizmoScaleComponent::DuplicateSubObjects()
{
    Super::DuplicateSubObjects();
}
