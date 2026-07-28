#include "./DisplayService.h"

#include <LilyGo_AMOLED.h>

LilyGo_AMOLED* DisplayService::board = nullptr;
bool DisplayService::displayOn = true;

uint8_t DisplayService::currentBrightness = 175;
uint8_t DisplayService::savedBrightness = 175;

void DisplayService::Begin(LilyGo_AMOLED *device)
{
    board = device;
    displayOn = true;

    currentBrightness = board->getBrightness();
    savedBrightness = currentBrightness;
}

void DisplayService::SetBrightness(uint8_t value)
{
    if (board == nullptr)
        return;

    board->setBrightness(value);
    currentBrightness = value;
}

uint8_t DisplayService::GetBrightness()
{
    return currentBrightness;
}

void DisplayService::Dim()
{
    if (!displayOn)
        return;

    savedBrightness = currentBrightness;
    SetBrightness(60);
}

void DisplayService::RestoreBrightness()
{
    if (!displayOn)
        return;

    SetBrightness(savedBrightness);
}

void DisplayService::TurnOn()
{
    displayOn = true;
}

void DisplayService::TurnOff()
{
    displayOn = false;
}

bool DisplayService::IsOn()
{
    return displayOn;
}