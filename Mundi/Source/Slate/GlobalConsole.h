#pragma once
#include <cstdarg>
#include <iostream>
#include "Object.h"
#include "UGlobalConsole.generated.h"

class UConsoleWidget;

/**
 * @brief Global Console Manager - replaces ImGuiConsole completely
 * Routes all console logging to our new ConsoleWidget
 */
UCLASS()
class UGlobalConsole : public UObject
{
public:
    GENERATED_REFLECTION_BODY()
        UGlobalConsole() {};
    static void Initialize();
    static void Shutdown();
    
    // Set the active console widget (called by UIWindowFactory)
    static void SetConsoleWidget(UConsoleWidget* InConsoleWidget);
    static UConsoleWidget* GetConsoleWidget();
    
    // Global logging functions (replaces ImGuiConsole functions)
    static void Log(const char* fmt, ...);
    static void LogV(const char* fmt, va_list args);

private:
    static UConsoleWidget* ConsoleWidget;
};

// Global functions for compatibility with existing code
extern "C" void ConsoleLog(const char* fmt, ...);
extern "C" void ConsoleLogV(const char* fmt, va_list args);

// UE_LOG macro replacement
#define UE_LOG(fmt, ...) UGlobalConsole::Log(fmt, ##__VA_ARGS__)
