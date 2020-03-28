#include "tgbot/net/TgLongPoll.h"

#include "tgbot/Api.h"
#include "tgbot/Bot.h"
#include "tgbot/EventHandler.h"

namespace TgBot
{

TgLongPoll::TgLongPoll(const Bot &bot, std::int32_t limit, std::int32_t timeout, const std::optional<std::vector<std::string>> &allowUpdates)
    : _api(bot.getApi())
    , _eventHandler(bot.getEventHandler())
    , _limit(limit)
    , _timeout(timeout)
    , _allowUpdates(allowUpdates)
{
}

void TgLongPoll::start()
{
    auto updates = _api.getUpdates(_lastUpdateId, _limit, _timeout, _allowUpdates);
    for (Update::Ptr& item : updates)
    {
        if (item->update_id >= _lastUpdateId)
            _lastUpdateId = item->update_id + 1;
        _eventHandler.handleUpdate(*item);
    }
}

}
