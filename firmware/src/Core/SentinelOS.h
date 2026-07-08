#pragma once

#include "../Managers/NavigationManager.h"
#include <stdint.h>
#include "../Managers/InputManager.h"
#include "../UI/ApplicationFrame.h"

enum class AppState
{
    Boot,
    Splash,
    Running
};

class SentinelOS
{
public:
    void Begin();
    void Update();

private:
    AppState currentState = AppState::Boot;
    uint32_t stateStartMs = 0;

    NavigationManager navigation;
    InputManager input;
    ApplicationFrame frame;

    void ChangeState(AppState newState);
    
};