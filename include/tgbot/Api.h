#pragma once

#include "tgbot/Types.h"

namespace TgBot
{

class CurlHttpClient;

/**
 * @brief This class executes telegram api methods.
 * Telegram docs: <https://core.telegram.org/bots/api#available-methods>
 */
class TGBOT_API Api
{
public:
    Api(const std::string &token, const CurlHttpClient &httpClient);

#include <methods.inl.h>

private:
    const std::string &token;
    const CurlHttpClient &httpClient;
};

}
