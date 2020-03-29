#pragma once

#include <string>
#include <vector>

// fwd decl
typedef void CURL;

namespace TgBot
{

class HttpRequestArgument;
using HttpRequestArguments = std::vector<HttpRequestArgument>;

/// This class makes http requests via libcurl.
class TGBOT_API CurlHttpClient
{
public:
    CurlHttpClient();
    ~CurlHttpClient();

    /// Get curl settings storage for fine tuning.
    CURL *getCurl() const { return curlSettings; }

    std::string makeRequest(const std::string &url, const HttpRequestArguments &args) const;
    std::string makeRequest(const std::string &url, const std::string &json) const;

private:
    CURL *curlSettings;
};

}
