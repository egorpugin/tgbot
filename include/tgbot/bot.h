#pragma once

#include "api.h"

namespace tgbot {

class curl_http_client;

/// This object holds other objects specific for this bot instance.
struct TGBOT_API bot {
    bot(const std::string &token);
    virtual ~bot();

    /// returns token for accessing api
    const std::string &token() const { return token_; }

    /// returns object which can execute Telegram Bot API methods
    const api &api() const { return api_; }

    /// used for fine tune setup of http connections
    curl_http_client &http_client() const { return *http_client_; }

    /// returns last processed update id + 1
    /// Designed to be executed in a loop.
    Integer process_updates(Integer offset = 0, Integer limit = default_update_limit, Integer timeout = default_update_timeout, const Vector<String> &allowed_updates = {});

    /// Calls process_updates() in a loop.
    /// Starts long poll. After new update will come, this method will parse it
    /// and send it to handle_update() method.
    void long_poll(Integer limit = default_update_limit, Integer timeout = default_update_timeout, const Vector<String> &allowed_updates = {});

    ///
    virtual void handle_update(const Update &update) = 0;

    const std::string &base_url() const { return base_url_; }
    const std::string &base_file_url() const { return base_file_url_; }
    std::string make_file_url(const std::string &file_path) const;

    int default_timeout() const { return 10; }

private:
    std::string token_;
    std::unique_ptr<curl_http_client> http_client_;
    struct api api_;
    std::string base_url_;
    std::string base_file_url_;

    static const int default_update_limit = 100;
    static const int default_update_timeout = 10;
};

}
