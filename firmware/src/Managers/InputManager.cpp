#include "InputManager.h"
#include <Arduino.h>

void InputManager::Begin(LilyGo_AMOLED *device)
{
    amoled = device;
}

InputEvent InputManager::Update()
{
    if (amoled == nullptr || !amoled->hasTouch())
    {
        return InputEvent::None;
    }

    int16_t x[1];
    int16_t y[1];

    bool hasPoint = amoled->getPoint(x, y, 1) > 0;
    uint32_t now = millis();

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

        return InputEvent::None;
    }

    if (wasPressed && (now - lastUpdateMs > 120))
    {
        wasPressed = false;

        uint32_t duration = now - startMs;
        int16_t deltaX = lastX - startX;
        int16_t deltaY = lastY - startY;

        if (duration > MAX_GESTURE_MS)
        {
            return InputEvent::None;
        }

        if (abs(deltaY) > SWIPE_LIMIT_Y)
        {
            return InputEvent::None;
        }

        if (deltaX <= -SWIPE_THRESHOLD_X)
        {
            return InputEvent::SwipeLeft;
        }

        if (deltaX >= SWIPE_THRESHOLD_X)
        {
            return InputEvent::SwipeRight;
        }

        return InputEvent::Tap;
    }

    return InputEvent::None;
}