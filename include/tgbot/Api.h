#pragma once

#include "tgbot/net/HttpClient.h"
#include "tgbot/Types.h"

#include <string>

namespace TgBot
{

class Bot;

/**
 * @brief This class executes telegram api methods.
 * Telegram docs: <https://core.telegram.org/bots/api#available-methods>
 *
 * @ingroup general
 */
class TGBOT_API Api
{
public:
    Api(const std::string &token, const HttpClient &httpClient);

#include <methods.inl.h>

private:
    const std::string token;
    const HttpClient &httpClient;
};

}
