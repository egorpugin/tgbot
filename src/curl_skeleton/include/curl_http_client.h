#pragma once

#include <curl/curl.h>
#include <tgbot/bot.h>
#include <primitives/templates.h>

#include <iostream>
#include <stdexcept>

static std::size_t curl_write_string(char *ptr, std::size_t size, std::size_t nmemb, void *userdata) {
    static_cast<std::string *>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

/// This class makes http requests via libcurl.
/// not mt safe
struct curl_http_client {
    curl_http_client() {
        curl_settings_ = curl_easy_init();

        // default options
        auto curl = curl_settings();

        curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);

        curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
        curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);
        // python bot also sets TCP_KEEPCNT 8
        // curl default is 9
    }
    ~curl_http_client() {
        curl_easy_cleanup(curl_settings());
    }

    std::string make_request(const std::string &url, const tgbot::http_request_arguments &args) const {
        auto curl = setup_connection(curl_settings(), url);
        curl_mime *mime = nullptr;
        if (!args.empty()) {
            mime = curl_mime_init(curl);
            for (auto &a : args) {
                auto part = curl_mime_addpart(mime);
                curl_mime_name(part, a.name.c_str());
                if (a.is_file()) {
                    auto fn = a.filename;
                    fn = fn.substr(fn.find_last_of("/\\") + 1);
                    curl_mime_filename(part, fn.c_str());
                    curl_mime_filedata(part, a.filename.c_str());
                    curl_mime_type(part, a.mimetype.c_str());
                } else {
                    curl_mime_data(part, a.value.c_str(), a.value.size());
                }
            }
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        }
        ScopeGuard se([mime] {
            curl_mime_free(mime);
        });
        return execute(curl);
    }
    std::string make_request(const std::string &url, const std::string &json) const {
        auto curl = setup_connection(curl_settings(), url);

        struct curl_slist *headers = nullptr;
        headers = curl_slist_append(headers, "Connection: keep-alive");
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        ScopeGuard se([headers] {
            curl_slist_free_all(headers);
        });

        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

        return execute(curl);
    }

    /// Get curl settings storage for fine tuning.
    CURL *curl_settings() const { return curl_settings_; }

    void set_timeout(long timeout) {
        if (timeout < connect_timeout)
            return;
        total_timeout = connect_timeout + timeout + 2;
    }

private:
    CURL *curl_settings_;
    long connect_timeout = 15;
    long total_timeout = 0;
    bool use_connection_pool = true;

    std::string execute(CURL *curl) const {
        std::string response;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string);

        auto res = curl_easy_perform(curl);
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (!use_connection_pool) {
            curl_easy_cleanup(curl);
        }

        if (res != CURLE_OK) {
            char *url = NULL;
            curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &url);
            throw std::runtime_error(std::format("curl error on url: {}: {}", url, curl_easy_strerror(res)));
        }
        if (http_code / 100 != 2) {
            if (http_code != 400) {
                std::cerr << std::format("http error: {}: {}\n", std::to_string(http_code), response);
            }
            //throw std::runtime_error(std::format("http error: {}: {}"s + std::to_string(http_code));
        }
        return response;
    }
    CURL *setup_connection(CURL *in, const std::string &url) const {
        auto curl = use_connection_pool ? in : curl_easy_duphandle(in);
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, total_timeout);
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr); // we must reset http headers, since we free them after use
        return curl;
    }
};
