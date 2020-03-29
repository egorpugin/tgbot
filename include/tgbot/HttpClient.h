#pragma once

#include <string>
#include <vector>

namespace TgBot
{

class HttpRequestArgument;
using HttpRequestArguments = std::vector<HttpRequestArgument>;

/// This class makes http requests.
class TGBOT_API HttpClient
{
public:
    virtual ~HttpClient() = default;

    /// Sends a request to the url.
    virtual std::string makeRequest(const std::string &url, const HttpRequestArguments &args) const = 0;
    virtual std::string makeRequest(const std::string &url, const std::string &json) const = 0;
};

}
