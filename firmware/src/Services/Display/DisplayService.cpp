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
    if (board == nullptr || !displayOn)
        return;

    constexpr uint8_t dimBrightness = 60;

    if (currentBrightness > dimBrightness)
    {
        savedBrightness = currentBrightness;
    }

    SetBrightness(dimBrightness);
}

void DisplayService::RestoreBrightness()
{
    if (!displayOn)
        return;

    SetBrightness(savedBrightness);
}

void DisplayService::TurnOn()
{
    if (board == nullptr || displayOn)
        return;

    uint8_t restoreLevel = savedBrightness;

    if (restoreLevel == 0)
    {
        restoreLevel = 175;
    }

    board->setBrightness(restoreLevel);
    currentBrightness = restoreLevel;
    displayOn = true;
}

void DisplayService::TurnOff()
{
    if (board == nullptr || !displayOn)
        return;

    if (currentBrightness > 0)
    {
        savedBrightness = currentBrightness;
    }

    board->setBrightness(0);
    currentBrightness = 0;
    displayOn = false;
}

bool DisplayService::IsOn()
{
    return displayOn;
}