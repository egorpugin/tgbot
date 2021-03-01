#include <tgbot/bot.h>

#include <tgbot/curl_http_client.h>

namespace tgbot {

bot::bot(const std::string &token)
    : token_(token)
    , http_client_(std::make_unique<curl_http_client>())
    , api_(*this)
{
}

}
