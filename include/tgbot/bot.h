#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace tgbot {

using Boolean = bool;
using Integer = std::int64_t;
using Float = double;
using String = std::string;

template <class T>
using Optional = std::optional<T>;
template <class ... Args>
using Ptr = std::unique_ptr<Args...>;
template <class ... Args>
using Variant = std::variant<Args...>;
template <class ... Args>
using Vector = std::vector<Args...>;

template <class ... PtrArgs, class ... Args>
auto createPtr(Args && ... args) {
    return std::make_unique<PtrArgs...>(args...);
}

#define TGBOT_TYPE_API TGBOT_API
#include <types.inl.h>

namespace detail {

/// Used to send files using their filenames in some requests.
struct http_request_argument {
    std::string name;
    std::string value;
    std::string filename;
    std::string mimetype;

    bool is_file() const { return !filename.empty(); }
};

// use range?
using http_request_arguments = std::span<http_request_argument>;

struct TGBOT_API http_client {
    virtual ~http_client() = 0;

    virtual std::string make_request(const std::string &url, const http_request_arguments &args) const = 0;
    virtual std::string make_request(const std::string &url, const std::string &json) const = 0;
};

} // namespace detail

struct bot;

/// This class executes telegram api methods.
/// Telegram docs: <https://core.telegram.org/bots/api#available-methods>
struct TGBOT_API api {
    api(const bot &b) : b(b) {}

#include <methods.inl.h>

private:
    const bot &b;
};

/// This object holds other objects specific for this bot instance.
struct TGBOT_API bot {
    bot(const std::string &t, const detail::http_client &c) : token_(t), http_client_(c), api_(*this) {}
    ~bot() = default;

    /// returns token for accessing api
    const std::string &token() const { return token_; }

    /// returns object which can execute Telegram Bot API methods
    const api &api() const { return api_; }

    /// used for fine tune setup of http connections
    const detail::http_client &http_client() const { return http_client_; }

    const std::string &base_url() const { return base_url_; }
    const std::string &base_file_url() const { return base_file_url_; }
    std::string make_file_url(const std::string &file_path) const { return base_file_url_ + token_ + "/" + file_path; }

private:
    std::string token_;
    const detail::http_client &http_client_;
    struct api api_;
    std::string base_url_{ "https://api.telegram.org/bot" };
    std::string base_file_url_{ "https://api.telegram.org/file/bot" };
};

}
