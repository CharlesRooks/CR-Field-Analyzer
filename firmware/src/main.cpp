#include <Arduino.h>
#include "Core/SentinelOS.h"

SentinelOS sentinel;

void setup()
{
    sentinel.Begin();
}

void loop()
{
    sentinel.Update();
}