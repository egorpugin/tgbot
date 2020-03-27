#include "tgbot/Api.h"

#include "tgbot/tools/StringTools.h"

#include <nlohmann/json.hpp>
#include <google/protobuf/util/json_util.h>

#include <cstdint>
#include <string>
#include <vector>
#include <utility>

#define GET_SIMPLE_FIELD(f)                                                                                            \
    if (j.contains(#f))                                                                                                \
        r->f = j[#f]

#define GET_FIELD(f, t)                                                                                                \
    if (j.contains(#f))                                                                                                \
        r->f = create##t(j[#f])

#define GET_ARRAY(f, t)                                                                                                \
    if (j.contains(#f))                                                                                                \
        for (const auto &i : j[#f])                                                                                    \
            r->f.push_back(create##t(i))

#define GET_RETURN_VALUE_ARRAY(t)                                                                                      \
    for (const auto& i : j)                                                                                            \
    r.push_back(create##t(i))

namespace TgBot {

Api::Api(std::string token, const HttpClient& httpClient)
    : _token(std::move(token)), _httpClient(httpClient)/*, _tgTypeParser()*/ {
}

//#include <methods.inl.cpp>

api::User Api::getMe(const api::getMeRequest &req) const
{
    std::string json;
    auto r = google::protobuf::util::MessageToJsonString(req, &json);
    if (!r.ok())
        throw std::runtime_error(r.ToString());
    auto jresp = sendRequest("getMe", json);
    api::User resp;
    r = google::protobuf::util::JsonStringToMessage(jresp, &resp);
    if (!r.ok())
        throw std::runtime_error(r.ToString());
    return resp;
}

String Api::sendRequest(const std::string& method, const std::string &json) const {
    std::string url = "https://api.telegram.org/bot";
    url += _token;
    url += "/";
    url += method;

    std::string serverResponse = _httpClient.makeRequest(url, json);
    if (!serverResponse.compare(0, 6, "<html>")) {
        throw std::runtime_error("tgbot-cpp library have got html page instead of json response. Maybe you entered wrong bot token.");
    }

    auto result = nlohmann::json::parse(serverResponse);
    if (result["ok"] == true) {
        return result["result"].dump();
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
