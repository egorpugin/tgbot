#include <tgbot/api.h>

#include "http_request_argument.h"

#include <tgbot/bot.h>
#include <tgbot/curl_http_client.h>

#include <nlohmann/json.hpp>

#define FROM_JSON(name) from_json(j, #name, v.name)
#define TO_JSON(name) to_json(j, #name, r.name)
#define TO_JSON2(name) to_json(j, #name, name)
#define TO_REQUEST_ARG(name) to_request_argument(args, #name, name)
#define SEND_REQUEST(method, var) send_request(b, #method, var)

namespace tgbot
{

template <class T>
static nlohmann::json send_request(const bot &bot, const char *method, const T &args)
{
    auto url = bot.base_url();
    url += bot.token();
    url += "/";
    url += method;

    auto response = bot.http_client().make_request(url, args);
    if (!response.compare(0, 6, "<html>"))
        throw std::runtime_error("tgbot-cpp library have got html page instead of json response. Maybe you entered wrong bot token.");

    // reset timeout after call
    bot.http_client().set_timeout(bot.default_timeout());

    auto result = nlohmann::json::parse(response);
    if (result["ok"] == true)
        return std::move(result["result"]);
    else
        throw std::runtime_error(result["description"].template get<std::string>());
}

#include "api_templates.h"
#include <methods.inl.cpp>

}