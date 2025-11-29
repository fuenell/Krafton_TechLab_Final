#pragma once
#include "UIWindow.h"
#include "UExperimentalFeatureWindow.generated.h"

/**
 * @brief 현재 단계에서 필요하지 않은 기능들을 모아둔 Window
 * Key Input, Actor Termination Widget을 포함한다
 */
UCLASS()
class UExperimentalFeatureWindow : public UUIWindow
{
public:
	GENERATED_REFLECTION_BODY()

	UExperimentalFeatureWindow();
	void Initialize() override;
};
