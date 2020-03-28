#include "tgbot/Api.h"

#include <nlohmann/json.hpp>

#define FROM_JSON(name, var) if (j.contains(#name)) fromJson(j[#name], var)
#define TO_JSON(name, var) j[#name] = toJson(var)
#define SEND_REQUEST(api) sendRequest(httpClient, token, #api, j)

namespace TgBot
{

template <class T>
static void fromJson(const nlohmann::json &j, T &v)
{
    v = j;
}

template <class T>
static void fromJson(const nlohmann::json &j, Optional<T> &v)
{
    T t;
    fromJson(j, t);
    v = std::move(t);
}

template <class T>
static void fromJson(const nlohmann::json &j, Ptr<T> &v)
{
    v = createPtr<T>();
    fromJson(j, *v);
}

template <class T>
static void fromJson(const nlohmann::json &j, Vector<T> &v)
{
    for (auto &i : j)
    {
        T t;
        fromJson(i, t);
        v.emplace_back(std::move(t));
    }
}

//

template <class T>
static nlohmann::json toJson(const T &r)
{
    return r;
}

template <class T>
static nlohmann::json toJson(const Optional<T> &r)
{
    if (!r)
        return {};
    return toJson(*r);
}

template <class T>
static nlohmann::json toJson(const Ptr<T> &r)
{
    if (!r)
        return {};
    return toJson(*r);
}

template <class T>
static nlohmann::json toJson(const Vector<T> &r)
{
    nlohmann::json j;
    for (auto &v : r)
        j.push_back(toJson(v));
    return j;
}

template <class ... Args>
static nlohmann::json toJson(const Variant<Args...> &r)
{
    return std::visit([](auto &&r) { return toJson(r); }, r);
}

//

static nlohmann::json sendRequest(const HttpClient &c, const std::string &token, const std::string &method, const nlohmann::json &req)
{
    std::string url = "https://api.telegram.org/bot";
    url += token;
    url += "/";
    url += method;

    std::string serverResponse = c.makeRequest(url, req.dump());
    if (!serverResponse.compare(0, 6, "<html>"))
        throw std::runtime_error("tgbot-cpp library have got html page instead of json response. Maybe you entered wrong bot token.");

    auto result = nlohmann::json::parse(serverResponse);
    if (result["ok"] == true)
        return std::move(result["result"]);
    else
        throw std::runtime_error(result["description"].get<String>());
}

#include <methods.inl.cpp>

Api::Api(const std::string &token, const HttpClient &httpClient)
    : token(token), httpClient(httpClient)
{
}

/*string Api::downloadFile(const string& filePath, const std::vector<HttpReqArg>& args) const
{
    string url = "https://api.telegram.org/file/bot";
    url += _token;
    url += "/";
    url += filePath;

    string serverResponse = _httpClient.makeRequest(url, args);

    return serverResponse;
}*/

}
