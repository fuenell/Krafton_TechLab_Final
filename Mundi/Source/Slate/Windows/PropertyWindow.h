#pragma once
#include "UIWindow.h"
#include "UPropertyWindow.generated.h"

UCLASS()
class UPropertyWindow : public UUIWindow
{
public:
	GENERATED_REFLECTION_BODY()

	UPropertyWindow();
	virtual void Initialize() override;
};