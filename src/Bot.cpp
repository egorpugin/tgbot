#include "tgbot/Bot.h"

#include "tgbot/EventBroadcaster.h"
#include "tgbot/net/CurlHttpClient.h"

#include <memory>
#include <string>

namespace TgBot
{

Bot::Bot(const std::string &token, const HttpClient &httpClient)
    : token(token)
    , api(token, httpClient)
    , eventBroadcaster(std::make_unique<EventBroadcaster>())
    , eventHandler(getEvents())
{
}

Bot::~Bot()
{
}

HttpClient &Bot::getDefaultHttpClient()
{
    static CurlHttpClient instance;
    return instance;
}

}
