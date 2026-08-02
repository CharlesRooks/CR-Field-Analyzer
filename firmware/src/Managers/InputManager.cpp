#include "InputManager.h"
#include <Arduino.h>

#include "../Core/Messaging/MessageBus.h"

void InputManager::Begin(LilyGo_AMOLED *device)
{
    amoled = device;
}

void InputManager::Update()
{
    if (amoled == nullptr || !amoled->hasTouch())
    {
        return;
    }

    int16_t x[1];
    int16_t y[1];

    const bool hasPoint = amoled->getPoint(x, y, 1) > 0;
    const uint32_t now = millis();

    if (hasPoint)
    {
        if (!wasPressed)
        {
            wasPressed = true;
            startX = x[0];
            startY = y[0];
            startMs = now;
        }

        lastX = x[0];
        lastY = y[0];
        lastUpdateMs = now;

        return;
    }

    if (wasPressed && (now - lastUpdateMs > 120))
    {
        wasPressed = false;

        const uint32_t duration = now - startMs;
        const int16_t deltaX = lastX - startX;
        const int16_t deltaY = lastY - startY;

        if (duration > MAX_GESTURE_MS)
        {
            return;
        }

        if (abs(deltaY) > SWIPE_LIMIT_Y)
        {
            return;
        }

        if (deltaX <= -SWIPE_THRESHOLD_X)
        {
            PublishEvent(InputEvent::SwipeLeft);
            return;
        }

        if (deltaX >= SWIPE_THRESHOLD_X)
        {
            PublishEvent(InputEvent::SwipeRight);
            return;
        }

        PublishEvent(InputEvent::Tap);
    }
}

void InputManager::PublishEvent(InputEvent event)
{
    if (event == InputEvent::None)
    {
        return;
    }

    Message activityMessage{};
    activityMessage.type = MessageType::UserActivity;
    activityMessage.timestampMs = millis();

    MessageBus::Publish(activityMessage);

    Message inputMessage{};
    inputMessage.type = MessageType::InputEvent;
    inputMessage.timestampMs = millis();
    inputMessage.inputEvent = event;

    MessageBus::Publish(inputMessage);
}