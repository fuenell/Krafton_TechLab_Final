#pragma once
#include "UIWindow.h"
#include "UControlPanelWindow.generated.h"

class UCamera;
class ULevelManager;
class AActor;

/**
 * @brief 전반적인 컨트롤을 담당하는 Panel Window
 * FPS Status, Actor Spawn, Level IO, Camera Control 기능이 포함되어 있다
 */
UCLASS()
class UControlPanelWindow : public UUIWindow
{
public:
	GENERATED_REFLECTION_BODY()

	UControlPanelWindow();
	void Initialize() override;
};
