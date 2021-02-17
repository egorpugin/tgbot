#include "tgbot/Api.h"

#include "HttpRequestArgument.h"
#include "tgbot/Bot.h"
#include "tgbot/CurlHttpClient.h"

#include <nlohmann/json.hpp>

#define FROM_JSON(name) if (j.contains(#name)) v.name = fromJson<decltype(v.name)>(j[#name])
#define TO_JSON(name, var) if (auto v = toJson(var); !v.is_null()) j[#name] = v
#define TO_REQUEST_ARG(name) if (auto v = toRequestArgument(#name, name); v) args.push_back(std::move(*v))
#define SEND_REQUEST(api, var) sendRequest(bot, #api, var)

namespace tgbot
{

template <class T>
static nlohmann::json sendRequest(const Bot &bot, const std::string &method, const T &args)
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
