#pragma once

#include <boost/asio.hpp>
#include <curl/curl.h>

#include <queue>
#include <string>
#include <vector>

namespace tgbot {

struct http_request_argument;
using http_request_arguments = std::vector<http_request_argument>;

/// This class makes http requests via libcurl.
/// not mt safe
struct TGBOT_API curl_http_client {
    curl_http_client();
    ~curl_http_client();

    std::string make_request(const std::string &url, const http_request_arguments &args) const;
    std::string make_request(const std::string &url, const std::string &json) const;
    boost::asio::awaitable<std::string> make_request_async(const std::string &url, const http_request_arguments &args) const;
    boost::asio::awaitable<std::string> make_request_async(const std::string &url, const std::string &json) const;

    /// Get curl settings storage for fine tuning.
    CURL *curl_settings() const { return curl_settings_; }
    //CURLM *curl_multi_settings() const { return multi_handle; }

    /// enable async mode
    void set_io_context(boost::asio::io_context &io_context);
    //boost::asio::io_context &io_context() { return *io_context_; }
    bool async() const { return io_context_; }

    void set_timeout(long timeout);

private:
    CURL *curl_settings_;
    CURLM *multi_handle = nullptr;
    mutable int net_delay_on_error = 1;
    long connect_timeout = 5;
    long read_timeout = 5;
    bool use_connection_pool_ = true;

    std::string execute(CURL *curl) const;
    boost::asio::awaitable<std::string> execute_async(CURL *curl) const;
    CURL *setup_connection(CURL *in, const std::string &url) const;
    bool use_connection_pool() const;
    std::tuple<CURL *, void *> prepare_request(const std::string &url, const http_request_arguments &args) const;
    std::tuple<CURL *, void *> prepare_request(const std::string &url, const std::string &json) const;

    // multi interface
    boost::asio::io_context *io_context_ = nullptr;
    std::unique_ptr<boost::asio::deadline_timer> timer;
    std::unordered_map<curl_socket_t, std::unique_ptr<boost::asio::ip::tcp::socket>> sockmap;
    int still_running = 0;
    mutable std::queue<boost::asio::awaitable<void>> awaitables;
public:
    static int multi_timer_cb(CURLM *multi, long timeout_ms, curl_http_client *client);
    boost::asio::awaitable<void> multi_timer_cb2();
    static int sock_cb(CURL *e, curl_socket_t s, int what, curl_http_client *client, void *sockp);
    void timer_cb();
    curl_socket_t open_socket(curlsocktype purpose, struct curl_sockaddr *address);
    int close_socket(curl_socket_t item);
    void check_multi_info();
    void remsock(int *f);
    void setsock(int *fdp, curl_socket_t s, CURL *e, int act, int oldact);
    void addsock(curl_socket_t s, CURL *easy, int action);
    void event_cb(curl_socket_t s, int action, const boost::system::error_code &error, int *fdp);
    boost::asio::awaitable<void> event_cb2(
        boost::asio::ip::tcp::socket &tcp_socket, boost::asio::ip::tcp::socket::wait_type wt,
        curl_socket_t s, int action, int *fdp);
};

}
