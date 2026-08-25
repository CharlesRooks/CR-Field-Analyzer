#include "NavigationManager.h"
#include "../Core/Messaging/MessageBus.h"
#include "../Services/USB/UsbStorageService.h"
#include <Arduino.h>

NavigationManager *NavigationManager::instance = nullptr;
bool NavigationManager::gestureNavigationEnabled = true;

void NavigationManager::Begin(lv_obj_t *area)
{
    contentArea = area;
    instance = this;
    gestureNavigationEnabled = true;

    if (!MessageBus::Subscribe(
        MessageType::InputEvent,
        NavigationManager::HandleMessage))
    {
        Serial.println(
            "NavigationManager: InputEvent "
            "subscription failed");
    }

    scan.Begin();

}

void NavigationManager::Show(ScreenID screen)
{
    if (contentArea == nullptr)
    {
        return;
    }

    lv_obj_clean(contentArea);

    current = screen;

    switch (screen)
    {
        case ScreenID::Dashboard:
            currentScreen = &dashboard;
            break;

        case ScreenID::Scan:
            currentScreen = &scan;
            break;

        case ScreenID::Tools:
            currentScreen = &tools;
            break;

        case ScreenID::Settings:
            currentScreen = &settings;
            break;

        default:
            currentScreen = &dashboard;
            current = ScreenID::Dashboard;
            break;
    }

    currentScreen->Show(contentArea);

    Message message{};
    message.type = MessageType::NavigationChanged;
    message.timestampMs = millis();
    message.screenId = current;

    MessageBus::Publish(message);
    
}

void NavigationManager::Next()
{
    int next = static_cast<int>(current) + 1;

    if (next >= static_cast<int>(ScreenID::Count))
    {
        next = 0;
    }

    Show(static_cast<ScreenID>(next));
}

void NavigationManager::Previous()
{
    int previous = static_cast<int>(current) - 1;

    if (previous < 0)
    {
        previous = static_cast<int>(ScreenID::Count) - 1;
    }

    Show(static_cast<ScreenID>(previous));
}

void NavigationManager::Update()
{
    if (currentScreen != nullptr)
    {
        currentScreen->Update();
    }
}

void NavigationManager::SetGestureNavigationEnabled(bool enabled)
{
    gestureNavigationEnabled = enabled;
}

bool NavigationManager::IsGestureNavigationEnabled()
{
    return gestureNavigationEnabled;
}

void NavigationManager::HandleMessage(const Message &message)
{
    if (instance == nullptr ||
        message.type != MessageType::InputEvent)
    {
        return;
    }

    // Keep SentinelOS on the Tools page while the SD card is exposed
    // to the host. This prevents navigation into views that may read
    // storage while USB Mass Storage is active.
    if (UsbStorageService::IsActive())
    {
        return;
    }

    // Measurement Setup and other modal workflows can explicitly block
    // left/right page swipes. Floor Plan placement uses horizontal drag
    // gestures, and allowing those gestures to reach NavigationManager
    // would silently change the page underneath the modal overlay.
    if (!gestureNavigationEnabled)
    {
        return;
    }

    if (message.inputEvent == InputEvent::SwipeLeft)
    {
        instance->Next();
    }
    else if (message.inputEvent == InputEvent::SwipeRight)
    {
        instance->Previous();
    }
}