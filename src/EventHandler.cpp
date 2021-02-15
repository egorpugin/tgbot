#include "tgbot/EventHandler.h"

#include "tgbot/EventBroadcaster.h"

#include <algorithm>

namespace tgbot
{

void EventHandler::handleUpdate(const Update &update) const
{
    if (update.inline_query)
        broadcaster.broadcastInlineQuery(*update.inline_query);
    if (update.chosen_inline_result)
        broadcaster.broadcastChosenInlineResult(*update.chosen_inline_result);
    if (update.callback_query)
        broadcaster.broadcastCallbackQuery(*update.callback_query);
    if (update.message)
        handleMessage(*update.message);
}

void EventHandler::handleMessage(const Message &message) const
{
    broadcaster.broadcastAnyMessage(message);

    if (!message.text)
        return;

    auto &text = *message.text;
    if (text.find("/") != 0)
    {
        broadcaster.broadcastNonCommandMessage(message);
        return;
    }

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
    if (!broadcaster.broadcastCommand(command, message))
        broadcaster.broadcastUnknownCommand(message);
}

}
