#pragma once

#include "tgbot/HttpClient.h"
#include "tgbot/Types.h"

namespace TgBot
{

/**
 * @brief This class executes telegram api methods.
 * Telegram docs: <https://core.telegram.org/bots/api#available-methods>
 */
class TGBOT_API Api
{
public:
    Api(const std::string &token, const HttpClient &httpClient);

#include <methods.inl.h>

private:
    const std::string &token;
    const HttpClient &httpClient;
};

}
