#pragma once

#include "api.h"

namespace tgbot {

struct curl_http_client;

/// This object holds other objects specific for this bot instance.
struct TGBOT_API bot {
    bot(const std::string &token);
    ~bot();

    /// returns token for accessing api
    const std::string &token() const { return token_; }

    /// returns object which can execute Telegram Bot API methods
    const api &api() const { return api_; }

    /// used for fine tune setup of http connections
    curl_http_client &http_client() const { return *http_client_; }

    const std::string &base_url() const { return base_url_; }
    const std::string &base_file_url() const { return base_file_url_; }
    std::string make_file_url(const std::string &file_path) const;

    //int default_timeout() const { return 10; }

private:
    std::string token_;
    std::unique_ptr<curl_http_client> http_client_;
    struct api api_;
    std::string base_url_;
    std::string base_file_url_;
};

}
