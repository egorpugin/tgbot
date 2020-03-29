#include "tgbot/Api.h"

#include "HttpRequestArgument.h"
#include "tgbot/CurlHttpClient.h"

#include <nlohmann/json.hpp>

#define FROM_JSON(name, var) if (j.contains(#name)) fromJson(j[#name], var)
#define TO_JSON(name, var) if (auto v = toJson(var); !v.is_null()) j[#name] = v
#define TO_REQUEST_ARG(name) if (auto v = toRequestArgument(#name, name); v) args.push_back(std::move(*v))
#define SEND_REQUEST(api, var) sendRequest(httpClient, token, #api, var)

template <class T>
static nlohmann::json sendRequest(const TgBot::CurlHttpClient &c, const std::string &token, const std::string &method, const T &args)
{
    std::string url = "https://api.telegram.org/bot";
    // std::string url = "https://api.telegram.org/file/bot"; // file downloads
    url += token;
    url += "/";
    url += method;

    std::string serverResponse = c.makeRequest(url, args);
    if (!serverResponse.compare(0, 6, "<html>"))
        throw std::runtime_error("tgbot-cpp library have got html page instead of json response. Maybe you entered wrong bot token.");

    auto result = nlohmann::json::parse(serverResponse);
    if (result["ok"] == true)
        return std::move(result["result"]);
    else
        throw std::runtime_error(result["description"].get<std::string>());
}

namespace TgBot
{

#include "ApiTemplates.h"
#include <methods.inl.cpp>

Api::Api(const std::string &token, const CurlHttpClient &httpClient)
    : token(token), httpClient(httpClient)
{
}

}
