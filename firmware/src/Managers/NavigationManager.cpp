#include "NavigationManager.h"
#include "../Core/Messaging/MessageBus.h"
#include <Arduino.h>

NavigationManager *NavigationManager::instance = nullptr;

void NavigationManager::Begin(lv_obj_t *area)
{
    contentArea = area;
    instance = this;

    MessageBus::Subscribe(
        MessageType::InputEvent,
        NavigationManager::HandleMessage);
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

void NavigationManager::HandleMessage(const Message &message)
{
    if (instance == nullptr ||
        message.type != MessageType::InputEvent)
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