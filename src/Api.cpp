#include "tgbot/Api.h"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#define GET_RETURN_VALUE_ARRAY(t)                                                                                      \
    for (const auto& i : j)                                                                                            \
    r.push_back(create##t(i))

#define FROM_JSON(name, var) if (j.contains(#name)) fromJson(j[#name], var)
#define TO_JSON(name, var) j[#name] = toJson(var)

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
    v = t;
}

template <class T>
static void fromJson(const nlohmann::json &j, Ptr<T> &v)
{
    v = std::make_shared<T>();
    fromJson(j, *v);
}

template <class T>
static void fromJson(const nlohmann::json &j, Vector<T> &v)
{
    for (auto &i : j)
    {
        T t;
        fromJson(i, t);
        v.push_back(t);
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

template <class T>
static Ptr<T> toPtr(const nlohmann::json &j)
{
    return std::make_shared<T>(std::move(fromJson<T>(j)));
}

#include <methods.inl.cpp>

Api::Api(const std::string &token, const HttpClient &httpClient)
    : _token(token), _httpClient(httpClient)
{
}

nlohmann::json Api::sendRequest(const std::string& method, const nlohmann::json &req) const {
    std::string url = "https://api.telegram.org/bot";
    url += _token;
    url += "/";
    url += method;

    std::string serverResponse = _httpClient.makeRequest(url, req.dump());
    if (!serverResponse.compare(0, 6, "<html>")) {
        throw std::runtime_error("tgbot-cpp library have got html page instead of json response. Maybe you entered wrong bot token.");
    }

    auto result = nlohmann::json::parse(serverResponse);
    if (result["ok"] == true) {
        return std::move(result["result"]);
    } else {
        throw std::runtime_error(result["description"].get<String>());
    }
}

/*string Api::downloadFile(const string& filePath, const std::vector<HttpReqArg>& args) const {
    string url = "https://api.telegram.org/file/bot";
    url += _token;
    url += "/";
    url += filePath;

    string serverResponse = _httpClient.makeRequest(url, args);

    return serverResponse;
}*/

}
