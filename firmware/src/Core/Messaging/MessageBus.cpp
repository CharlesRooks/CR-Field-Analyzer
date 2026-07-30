#include "MessageBus.h"

MessageBus::Subscription
    MessageBus::subscriptions[MessageBus::MaxSubscribers];

uint8_t MessageBus::subscriptionCount = 0;

void MessageBus::Begin()
{
    subscriptionCount = 0;

    for (uint8_t index = 0;
         index < MaxSubscribers;
         ++index)
    {
        subscriptions[index].type = MessageType::None;
        subscriptions[index].handler = nullptr;
    }
}

bool MessageBus::Subscribe(
    MessageType type,
    MessageHandler handler)
{
    if (handler == nullptr)
        return false;

    if (subscriptionCount >= MaxSubscribers)
        return false;

    subscriptions[subscriptionCount].type = type;
    subscriptions[subscriptionCount].handler = handler;

    ++subscriptionCount;

    return true;
}

void MessageBus::Publish(const Message &message)
{
    for (uint8_t index = 0;
         index < subscriptionCount;
         ++index)
    {
        const Subscription &subscription =
            subscriptions[index];

        if (subscription.handler == nullptr)
            continue;

        if (subscription.type != message.type)
            continue;

        subscription.handler(message);
    }
}