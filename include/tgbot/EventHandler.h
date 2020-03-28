#pragma once

#include "tgbot/Types.h"

namespace TgBot
{

class EventBroadcaster;

class TGBOT_API EventHandler
{
public:
    explicit EventHandler(const EventBroadcaster &broadcaster) : broadcaster(broadcaster) {}

    void handleUpdate(const Update &update) const;

private:
    const EventBroadcaster &broadcaster;

    void handleMessage(const Message &message) const;
};

}
