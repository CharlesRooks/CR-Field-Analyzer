#include "PowerManager.h"
#include <Arduino.h>

#include "../Services/Power/PowerService.h"
#include "../Services/Display/DisplayService.h"

PowerPolicy PowerManager::usbPolicy;
PowerPolicy PowerManager::batteryPolicy;
uint32_t PowerManager::lastActivityMs = 0;
bool PowerManager::displayDimmed = false;

const PowerPolicy *PowerManager::activePolicy =
    &PowerManager::batteryPolicy;

void PowerManager::Begin()
{
    // USB policy: remain fully active while connected to USB.
    usbPolicy.automaticPowerSavingEnabled = false;
    usbPolicy.allowAutomaticSleepOnUsb = false;
    usbPolicy.allowDisplayDimming = false;
    usbPolicy.allowDisplaySleep = false;
    usbPolicy.allowDeepSleep = false;
    usbPolicy.dimTimeoutMs = 0;
    usbPolicy.displayOffTimeoutMs = 0;
    usbPolicy.deepSleepTimeoutMs = 0;

    // Battery policy: conserve power during field operation.
    batteryPolicy.automaticPowerSavingEnabled = true;
    batteryPolicy.allowAutomaticSleepOnUsb = false;
    batteryPolicy.allowDisplayDimming = true;
    batteryPolicy.allowDisplaySleep = true;
    batteryPolicy.allowDeepSleep = true;
    batteryPolicy.dimTimeoutMs = 30000;
    batteryPolicy.displayOffTimeoutMs = 60000;
    batteryPolicy.deepSleepTimeoutMs = 120000;

    SelectActivePolicy();

    lastActivityMs = millis();
    SelectActivePolicy();
}

void PowerManager::NotifyActivity()
{
    lastActivityMs = millis();
    RestoreDisplayBrightness();
}

uint32_t PowerManager::GetIdleTimeMs()
{
    return millis() - lastActivityMs;
}

void PowerManager::Update()
{
    SelectActivePolicy();
    ApplyDisplayPolicy();
}

const PowerPolicy &PowerManager::GetActivePolicy()
{
    return *activePolicy;
}

void PowerManager::SelectActivePolicy()
{
    if (PowerService::IsUSBConnected())
    {
        activePolicy = &usbPolicy;
    }
    else
    {
        activePolicy = &batteryPolicy;
    }
}
void PowerManager::ApplyDisplayPolicy()
{
    if (activePolicy == nullptr)
    {
        return;
    }

    if (!activePolicy->automaticPowerSavingEnabled ||
        !activePolicy->allowDisplayDimming)
    {
        RestoreDisplayBrightness();
        return;
    }

    if (!displayDimmed &&
        GetIdleTimeMs() >= activePolicy->dimTimeoutMs)
    {
        DisplayService::SetBrightness(60);
        displayDimmed = true;
    }
}

void PowerManager::RestoreDisplayBrightness()
{
    if (!displayDimmed)
    {
        return;
    }

    DisplayService::SetBrightness(175);
    displayDimmed = false;
}