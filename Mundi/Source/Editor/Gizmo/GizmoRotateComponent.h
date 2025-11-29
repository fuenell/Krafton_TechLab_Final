#pragma once
#include "GizmoArrowComponent.h"
#include "UGizmoRotateComponent.generated.h"
UCLASS()
class UGizmoRotateComponent :public UGizmoArrowComponent
{
public:
    GENERATED_REFLECTION_BODY()
    UGizmoRotateComponent();

protected:
    ~UGizmoRotateComponent() override;

public:
    

    void DuplicateSubObjects() override;
};

