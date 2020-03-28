#pragma once

#include "HttpClient.h"

// fwd decl
typedef void CURL;

namespace TgBot
{

/// This class makes http requests via libcurl.
class TGBOT_API CurlHttpClient : public HttpClient
{
public:
    CurlHttpClient();
    ~CurlHttpClient() override;

    /// Get curl settings storage for fine tuning.
    CURL *getCurl() const { return curlSettings; }

    std::string makeRequest(const std::string &url, const std::string &json) const override;

private:
    CURL *curlSettings;
};

}
