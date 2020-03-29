#include "tgbot/Bot.h"

#include "tgbot/EventBroadcaster.h"
#include "tgbot/CurlHttpClient.h"

#include <memory>
#include <string>

namespace TgBot
{

Bot::Bot(const std::string &token)
    : token(token)
    , httpClient(std::make_unique<CurlHttpClient>())
    , api(token, getHttpClient())
    , eventBroadcaster(std::make_unique<EventBroadcaster>())
    , eventHandler(getEvents())
{
}

Bot::~Bot()
{
}

Integer Bot::processUpdates(Integer offset, Integer limit, Integer timeout, const Vector<String> &allowed_updates) const
{
    auto updates = api.getUpdates(offset, limit, timeout, allowed_updates);
    for (const auto &item : updates)
    {
        if (item->update_id >= offset)
            offset = item->update_id + 1;
        eventHandler.handleUpdate(*item);
    }
    return offset;
}

void Bot::longPoll(Integer limit, Integer timeout, const Vector<String> &allowed_updates) const
{
    Integer offset = 0;
    while (1)
        offset = processUpdates(offset, limit, timeout, allowed_updates);
}

}
