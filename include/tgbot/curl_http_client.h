#pragma once

#include <span>
#include <string>

// fwd decl
typedef void CURL;

namespace tgbot {

struct http_request_argument;
using http_request_arguments = std::span<http_request_argument>;

/// This class makes http requests via libcurl.
/// not mt safe
struct TGBOT_API curl_http_client {
    curl_http_client();
    ~curl_http_client();

    std::string make_request(const std::string &url, const http_request_arguments &args) const;
    std::string make_request(const std::string &url, const std::string &json) const;

    /// Get curl settings storage for fine tuning.
    CURL *curl_settings() const { return curl_settings_; }

    void set_timeout(long timeout);

private:
    CURL *curl_settings_;
    long connect_timeout = 5;
    long read_timeout = 5;
    bool use_connection_pool = true;

    std::string execute(CURL *curl) const;
    CURL *setup_connection(CURL *in, const std::string &url) const;
};

}
