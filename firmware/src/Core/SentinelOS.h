#pragma once

#include "../Managers/NavigationManager.h"
#include "../Managers/InputManager.h"
#include "../UI/ApplicationFrame.h"
#include "Messaging/MessageTypes.h"
#include <stdint.h>

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
    static SentinelOS *instance;
    static void HandleMessage(const Message &message);

    AppState currentState = AppState::Boot;
    uint32_t stateStartMs = 0;

    NavigationManager navigation;
    InputManager input;
    ApplicationFrame frame;

    void ChangeState(AppState newState);
    
};