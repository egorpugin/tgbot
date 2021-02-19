#include <tgbot/curl_http_client.h>

#include "http_request_argument.h"

#include <curl/curl.h>

#include <stdexcept>
#include <thread>

template <class F>
struct scope_exit {
    F f;
    scope_exit(F f) : f(f) {}
    ~scope_exit() { f(); }
};

namespace tgbot {

curl_http_client::curl_http_client() {
    curl_settings_ = curl_easy_init();

    // default options
    auto curl = curl_settings();

    curl_easy_setopt(curl, CURLOPT_TCP_NODELAY, 1L);

    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, connect_timeout);

    curl_easy_setopt(curl, CURLOPT_TCP_KEEPALIVE, 1L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPIDLE, 120L);
    curl_easy_setopt(curl, CURLOPT_TCP_KEEPINTVL, 30L);
    // python bot also sets TCP_KEEPCNT 8, but there is no such curl setting atm
}

curl_http_client::~curl_http_client() {
    curl_easy_cleanup(curl_settings());
}

void curl_http_client::set_timeout(long t) {
    if (t < connect_timeout)
        return;
    read_timeout = connect_timeout + t + 2;
}

std::string curl_http_client::make_request(const std::string &url, const std::string &json) const {
    auto curl = setup_connection(curl_settings(), url);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    scope_exit se([headers] { curl_slist_free_all(headers); });

    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

    return execute(curl);
}

std::string curl_http_client::make_request(const std::string &url, const http_request_arguments &args) const {
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
    scope_exit se([mime] { curl_mime_free(mime); });
    return execute(curl);
}

CURL *curl_http_client::setup_connection(CURL *in, const std::string &url) const {
    auto curl = use_connection_pool ? in : curl_easy_duphandle(in);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, read_timeout);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, nullptr); // we must reset http headers, since we free them after use
    return curl;
}

static std::size_t curl_write_string(char *ptr, std::size_t size, std::size_t nmemb, void *userdata) {
    static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string curl_http_client::execute(CURL *curl) const {
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string);

    auto res = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (!use_connection_pool)
        curl_easy_cleanup(curl);

    if (http_code >= 500 || res) {
        std::this_thread::sleep_for(std::chrono::seconds(net_delay_on_error));
        if (net_delay_on_error < 30)
            net_delay_on_error *= 2;
    }
    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));
    return response;
}

}
