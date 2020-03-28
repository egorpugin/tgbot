#include "tgbot/EventHandler.h"

#include "tgbot/EventBroadcaster.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

namespace TgBot
{

void EventHandler::handleUpdate(const Update::Ptr &update) const
{
    if (update->inline_query/* && *update->inline_query*/)
        _broadcaster.broadcastInlineQuery(*update->inline_query);
    if (update->chosen_inline_result)
        _broadcaster.broadcastChosenInlineResult(*update->chosen_inline_result);
    if (update->callback_query)
        _broadcaster.broadcastCallbackQuery(*update->callback_query);
    if (update->message)
        handleMessage(*update->message);
}

void EventHandler::handleMessage(const Message::Ptr &message) const
{
    _broadcaster.broadcastAnyMessage(message);

    auto &text = *message->text;
    if (text.find("/") == 0)
    {
        std::size_t splitPosition;
        std::size_t spacePosition = text.find(' ');
        std::size_t atSymbolPosition = text.find('@');
        if (spacePosition == std::string::npos) {
            if (atSymbolPosition == std::string::npos) {
                splitPosition = text.size();
            } else {
                splitPosition = atSymbolPosition;
            }
        } else if (atSymbolPosition == std::string::npos) {
            splitPosition = spacePosition;
        } else {
            splitPosition = std::min(spacePosition, atSymbolPosition);
        }
        std::string command = text.substr(1, splitPosition - 1);
        if (!_broadcaster.broadcastCommand(command, message)) {
            _broadcaster.broadcastUnknownCommand(message);
        }
    } else {
        _broadcaster.broadcastNonCommandMessage(message);
    }
}

}
