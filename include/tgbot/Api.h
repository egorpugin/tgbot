#ifndef TGBOT_CPP_API_H
#define TGBOT_CPP_API_H

#include "tgbot/net/HttpClient.h"
#include "tgbot/Types.h"

#include <tgapi.pb.h>

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace TgBot {

class Bot;

/**
 * @brief This class executes telegram api methods. Telegram docs: <https://core.telegram.org/bots/api#available-methods>
 *
 * @ingroup general
 */
class TGBOT_API Api {

typedef std::shared_ptr<std::vector<std::string>> StringArrayPtr;

friend class Bot;

public:
    Api(std::string token, const HttpClient& httpClient);

#include <methods.inl.h>

private:
    String sendRequest(const std::string& method, const std::string &json) const;

    const std::string _token;
    const HttpClient& _httpClient;
};

}

#endif //TGBOT_CPP_API_H
