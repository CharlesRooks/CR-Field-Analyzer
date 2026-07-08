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
            Serial.printf("Gesture start: X=%d Y=%d\n", startX, startY);
        }

        lastX = x[0];
        lastY = y[0];
        lastUpdateMs = now;

        Serial.printf("Touch: X=%d Y=%d\n", lastX, lastY);

        return InputEvent::None;
    }

    if (wasPressed && (now - lastUpdateMs > 120))
    {
        wasPressed = false;

        uint32_t duration = now - startMs;
        int16_t deltaX = lastX - startX;
        int16_t deltaY = lastY - startY;

        Serial.printf("Gesture end: DX=%d DY=%d Duration=%lu ms\n",
                      deltaX, deltaY, duration);

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
            Serial.println("Swipe LEFT");
            return InputEvent::SwipeLeft;
        }

        if (deltaX >= SWIPE_THRESHOLD_X)
        {
            Serial.println("Swipe RIGHT");
            return InputEvent::SwipeRight;
        }

        Serial.println("Tap");
        return InputEvent::Tap;
    }

    return InputEvent::None;
}