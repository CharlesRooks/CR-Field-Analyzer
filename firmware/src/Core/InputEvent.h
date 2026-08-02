#pragma once

#include <stdint.h>

enum class InputEvent : uint8_t
{
    None = 0,
    SwipeLeft,
    SwipeRight,
    Tap
};