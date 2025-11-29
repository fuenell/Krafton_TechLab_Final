#pragma once
#include "GizmoArrowComponent.h"
#include "UGizmoScaleComponent.generated.h"
UCLASS()
class UGizmoScaleComponent :
    public UGizmoArrowComponent
{
public:
    GENERATED_REFLECTION_BODY()
    UGizmoScaleComponent();

protected:
    ~UGizmoScaleComponent() override;

public:


    void DuplicateSubObjects() override;
};

