#include "tgbot/LongPoll.h"

#include "tgbot/Api.h"
#include "tgbot/Bot.h"
#include "tgbot/EventHandler.h"

namespace TgBot
{

LongPoll::LongPoll(const Bot &bot, std::int32_t limit, std::int32_t timeout, const std::vector<std::string> &allowUpdates)
    : api(bot.getApi())
    , eventHandler(bot.getEventHandler())
    , limit(limit)
    , timeout(timeout)
    , allowUpdates(allowUpdates)
{
}

void LongPoll::start()
{
    auto updates = api.getUpdates(lastUpdateId, limit, timeout, allowUpdates);
    for (const auto &item : updates)
    {
        if (item->update_id >= lastUpdateId)
            lastUpdateId = item->update_id + 1;
        eventHandler.handleUpdate(*item);
    }
}

}
