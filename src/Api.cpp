#include "tgbot/Api.h"

#include "HttpRequestArgument.h"
#include "tgbot/Bot.h"
#include "tgbot/CurlHttpClient.h"

#include <nlohmann/json.hpp>

#define FROM_JSON(name) from_json(j, #name, v.name)
#define TO_JSON(name) to_json(j, #name, r.name)
#define TO_JSON2(name) to_json(j, #name, name)
#define TO_REQUEST_ARG(name) to_request_argument(args, #name, name)
#define SEND_REQUEST(method, var) send_request(bot, #method, var)

namespace tgbot
{

template <class T>
static nlohmann::json send_request(const Bot &bot, const std::string &method, const T &args)
{
    auto url = bot.getBaseUrl();
    url += bot.getToken();
    url += "/";
    url += method;

    std::string serverResponse = bot.getHttpClient().makeRequest(url, args);
    if (!serverResponse.compare(0, 6, "<html>"))
        throw std::runtime_error("tgbot-cpp library have got html page instead of json response. Maybe you entered wrong bot token.");

    // reset timeout after call
    bot.getHttpClient().setTimeout(bot.getDefaultTimeout());

    auto result = nlohmann::json::parse(serverResponse);
    if (result["ok"] == true)
        return std::move(result["result"]);
    else
        throw std::runtime_error(result["description"].get<std::string>());
}

#include "ApiTemplates.h"
#include <methods.inl.cpp>

Api::Api(const Bot &bot)
    : bot(bot)
{
}

}
