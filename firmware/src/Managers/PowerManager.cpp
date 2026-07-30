#include "PowerManager.h"
#include <Arduino.h>

#include "../Services/Power/PowerService.h"
#include "../Services/Display/DisplayService.h"
#include "../Core/Messaging/MessageBus.h"

PowerPolicy PowerManager::usbPolicy;
PowerPolicy PowerManager::batteryPolicy;
uint32_t PowerManager::lastActivityMs = 0;

DisplayPowerState PowerManager::displayState =
    DisplayPowerState::Active;

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

    displayState = DisplayPowerState::Active;
    lastActivityMs = millis();

    SelectActivePolicy();

    MessageBus::Subscribe(
    MessageType::UserActivity,
    PowerManager::HandleMessage);
}

void PowerManager::NotifyActivity()
{
    lastActivityMs = millis();

    switch (displayState)
    {
        case DisplayPowerState::Active:
            break;

        case DisplayPowerState::Dimmed:
            DisplayService::RestoreBrightness();
            displayState = DisplayPowerState::Active;
            break;

        case DisplayPowerState::Off:
            DisplayService::TurnOn();
            displayState = DisplayPowerState::Active;
            break;
    }
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
        return;

    // Automatic power saving disabled.
    if (!activePolicy->automaticPowerSavingEnabled)
        return;

    const uint32_t idleTimeMs = millis() - lastActivityMs;

    switch (displayState)
    {
        case DisplayPowerState::Active:
        {
            // Dim display after inactivity.
            if (activePolicy->allowDisplayDimming &&
                idleTimeMs >= activePolicy->dimTimeoutMs)
            {
                DisplayService::Dim();
                displayState = DisplayPowerState::Dimmed;
                break;
            }

            // Turn display off after longer inactivity.
            if (activePolicy->allowDisplaySleep &&
                idleTimeMs >= activePolicy->displayOffTimeoutMs)
            {
                DisplayService::TurnOff();
                displayState = DisplayPowerState::Off;
            }

            break;
        }

        case DisplayPowerState::Dimmed:
        {
            // Turn display off after extended inactivity.
            if (activePolicy->allowDisplaySleep &&
                idleTimeMs >= activePolicy->displayOffTimeoutMs)
            {
                DisplayService::TurnOff();
                displayState = DisplayPowerState::Off;
            }

            break;
        }

        case DisplayPowerState::Off:
        {
            // Deep sleep will be handled here in the next phase.
            break;
        }
    }
}

void PowerManager::HandleMessage(const Message &message)
{
    if (message.type != MessageType::UserActivity)
        return;

    NotifyActivity();
}