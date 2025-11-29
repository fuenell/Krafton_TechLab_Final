#include "pch.h"
#include "CameraModifierBase.h"
#include "SceneView.h"

UCameraModifierBase::UCameraModifierBase()
{
}

void UCameraModifierBase::ApplyToView(float DeltaTime, FMinimalViewInfo* ViewInfo)
{
}

void UCameraModifierBase::CollectPostProcess(TArray<FPostProcessModifier>& OutModifiers)
{
}

