#include "tgbot/Bot.h"

#include "tgbot/EventBroadcaster.h"
#include "tgbot/net/CurlHttpClient.h"

#include <memory>
#include <string>

namespace TgBot {

Bot::Bot(std::string token, const HttpClient& httpClient)
    : _token(std::move(token))
    , _api(_token, httpClient)
    , _eventBroadcaster(std::make_unique<EventBroadcaster>())
    , _eventHandler(getEvents()) {
}

Bot::~Bot()
{
}

HttpClient& Bot::_getDefaultHttpClient() {
    static CurlHttpClient instance;
    return instance;
}

}
