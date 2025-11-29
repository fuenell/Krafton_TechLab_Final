#pragma once
#include "UIWindow.h"
#include "USceneWindow.generated.h"

class ULevelManager;

/**
 * @brief Scene Manager 역할을 제공할 Window
 * Scene hierarchy and management UI를 제공한다
 */
UCLASS()
class USceneWindow
	: public UUIWindow
{
public:
	GENERATED_REFLECTION_BODY()

    USceneWindow();
	void Initialize() override;
};
