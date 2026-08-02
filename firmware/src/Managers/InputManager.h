#pragma once

#include <Arduino.h>
#include <LilyGo_AMOLED.h>

#include "../Core/InputEvent.h"

class InputManager
{
public:
    void Begin(LilyGo_AMOLED *device);
    void Update();

private:
    LilyGo_AMOLED *amoled = nullptr;

    bool wasPressed = false;
    int16_t startX = 0;
    int16_t startY = 0;
    int16_t lastX = 0;
    int16_t lastY = 0;
    uint32_t startMs = 0;
    uint32_t lastUpdateMs = 0;

    static constexpr int16_t SWIPE_THRESHOLD_X = 60;
    static constexpr int16_t SWIPE_LIMIT_Y = 80;
    static constexpr uint32_t MAX_GESTURE_MS = 800;

      void PublishEvent(InputEvent event);
};