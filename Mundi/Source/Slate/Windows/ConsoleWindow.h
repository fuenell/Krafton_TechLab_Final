#pragma once
#include "UIWindow.h"
#include <cstdarg>
#include "UConsoleWindow.generated.h"

class UConsoleWidget;

/**
 * @brief Console Window containing ConsoleWidget
 * Replaces the old ImGuiConsole with new widget system
 */
UCLASS()
class UConsoleWindow : public UUIWindow
{
public:
    GENERATED_REFLECTION_BODY()

    UConsoleWindow();
    ~UConsoleWindow() override;

    void Initialize() override;
    
    // Console specific methods
    void AddLog(const char* fmt, ...);
    void ClearLog();
    
    // Accessors
    UConsoleWidget* GetConsoleWidget() const;

    UPROPERTY(EditAnywhere, Category = "Console")
    UConsoleWidget* ConsoleWidget;
};
