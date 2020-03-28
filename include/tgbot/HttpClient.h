#pragma once

#include <string>

namespace TgBot
{

/// This class makes http requests.
class TGBOT_API HttpClient
{
public:
    virtual ~HttpClient() = default;

    /// Sends a request to the url.
    virtual std::string makeRequest(const std::string &url, const std::string &json) const = 0;
};

}
