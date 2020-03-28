#ifndef TGBOT_CPP_API_H
#define TGBOT_CPP_API_H

#include "tgbot/net/HttpClient.h"
#include "tgbot/Types.h"

#include <nlohmann/json_fwd.hpp>

#include <string>

namespace TgBot {

class Bot;

/**
 * @brief This class executes telegram api methods. Telegram docs: <https://core.telegram.org/bots/api#available-methods>
 *
 * @ingroup general
 */
class TGBOT_API Api {

public:
    Api(const std::string &token, const HttpClient &httpClient);

#include <methods.inl.h>

private:
    nlohmann::json sendRequest(const std::string &method, const nlohmann::json &) const;

    const std::string _token;
    const HttpClient& _httpClient;
};

}

#endif //TGBOT_CPP_API_H
