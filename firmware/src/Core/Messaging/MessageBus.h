#pragma once

#include "MessageTypes.h"
#include <stdint.h>

using MessageHandler = void (*)(const Message &message);

class MessageBus
{
public:
    static constexpr uint8_t MaxSubscribers = 16;

    static void Begin();

    static bool Subscribe(
        MessageType type,
        MessageHandler handler);

    static void Publish(const Message &message);

private:
    struct Subscription
    {
        MessageType type = MessageType::None;
        MessageHandler handler = nullptr;
    };

    static Subscription subscriptions[MaxSubscribers];
    static uint8_t subscriptionCount;
};