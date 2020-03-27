#ifndef TGBOT_CPP_API_H
#define TGBOT_CPP_API_H

#include "tgbot/TgTypeParser.h"
#include "tgbot/net/HttpClient.h"
#include "tgbot/net/HttpReqArg.h"
#include "tgbot/Types.h"
//#include <boost/property_tree/ptree.hpp>

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
    nlohmann::json sendRequest(const std::string& method, const std::vector<HttpReqArg>& args = std::vector<HttpReqArg>()) const;

    const std::string _token;
    const HttpClient& _httpClient;
    const TgTypeParser _tgTypeParser;
};

}

#endif //TGBOT_CPP_API_H
