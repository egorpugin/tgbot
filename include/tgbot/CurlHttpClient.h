#pragma once

#include <string>
#include <vector>

// fwd decl
typedef void CURL;

namespace tgbot
{

class HttpRequestArgument;
using HttpRequestArguments = std::vector<HttpRequestArgument>;

/// This class makes http requests via libcurl.
/// not mt safe
class TGBOT_API CurlHttpClient
{
public:
    CurlHttpClient();
    ~CurlHttpClient();

    std::string makeRequest(const std::string &url, const HttpRequestArguments &args) const;
    std::string makeRequest(const std::string &url, const std::string &json) const;

    /// Get curl settings storage for fine tuning.
    CURL *getCurl() const { return curlSettings; }

    void setTimeout(long timeout);

private:
    CURL *curlSettings;
    mutable int net_delay_on_error = 1;
    long connect_timeout = 5;
    long read_timeout = 5;
    bool use_connection_pool = true;

    std::string execute(CURL *curl) const;
    CURL *setupConnection(CURL *in, const std::string &url) const;
};

}
