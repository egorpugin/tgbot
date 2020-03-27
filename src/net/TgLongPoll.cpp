#include "tgbot/net/TgLongPoll.h"

#include "tgbot/Api.h"
#include "tgbot/Bot.h"
#include "tgbot/EventHandler.h"

#include <cstdint>
#include <memory>
#include <vector>
#include <utility>

namespace TgBot {

TgLongPoll::TgLongPoll(const Api* api, const EventHandler* eventHandler, std::int32_t limit, std::int32_t timeout, std::optional<std::vector<std::string>> allowUpdates)
    : _api(api), _eventHandler(eventHandler), _limit(limit), _timeout(timeout),
      _allowUpdates(allowUpdates) {
}

TgLongPoll::TgLongPoll(const Bot& bot, std::int32_t limit, std::int32_t timeout, const std::optional<std::vector<std::string>>& allowUpdates) :
    TgLongPoll(&bot.getApi(), &bot.getEventHandler(), limit, timeout, allowUpdates) {
}

void TgLongPoll::start() {
    /*auto updates = _api->getUpdates(_lastUpdateId, _limit, _timeout, _allowUpdates);
    for (Update::Ptr& item : updates) {
        if (item->update_id >= _lastUpdateId) {
            _lastUpdateId = item->update_id + 1;
        }
        _eventHandler->handleUpdate(item);
    }*/
}

}
