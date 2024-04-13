#pragma once

//#include <boost/pfr.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#define TBGOT_TO_JSON_REQUEST_MACRO(f, ret)                                                                            \
    auto f(const f##Request &r) const {                                                                                \
        return send_request_to_json<ret>(r, #f);                                                                       \
    }

#define TBGOT_TO_REQUEST_ARGUMENT_REQUEST_MACRO(f, ret)                                                                \
    auto f(const f##Request &r) const {                                                                                \
        return send_request_to_arguments<ret>(r, #f);                                                                  \
    }

namespace tgbot {

using namespace std::literals;

using Boolean = bool;
using Integer = std::int64_t;
using Float = double;
using String = std::string;

template<typename T>
constexpr bool is_simple_type = std::disjunction_v<
    std::is_same<T, Boolean>,
    std::is_same<T, Integer>,
    std::is_same<T, Float>,
    std::is_same<T, String>
>;

template <typename T>
using Optional = std::optional<T>;
template <typename ... Args>
using Ptr = std::unique_ptr<Args...>;
template <typename ... Args>
using Variant = std::variant<Args...>;
template <typename ... Args>
using Vector = std::vector<Args...>;

template <typename ... PtrArgs, typename ... Args>
auto create_ptr(Args && ... args) {
    return std::make_unique<PtrArgs...>(args...);
}

// introspection helpers

// boost::pfr::tuple_size<f##Request>::value does not work in clang
// https://github.com/boostorg/pfr/issues/168
/*template <typename T>
struct tuple_size {
    static constexpr auto value = boost::pfr::tuple_size<T>::value;
};
template <auto I, typename T>
auto get_name() {
    return boost::pfr::get_name<I, T>();
}
void for_each_field(auto &&v, auto &&f) {
    boost::pfr::for_each_field(v, f);
}*/
template <typename T>
struct tuple_size {
    static constexpr auto value = T::number_of_fields;
};
template <auto I, typename T>
auto get_name() {
    return T::template get_field_name<I>();
}
void for_each_field(auto &&v, auto &&f) {
    v.for_each_field(f);
}

//

#include <types.inl.h>

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

/// This class executes telegram api methods.
/// Telegram docs: <https://core.telegram.org/bots/api#available-methods>
template <typename Bot>
struct api {
private:
    template <typename T> struct type {};

public:
    api(const Bot &b) : bot{b} {}

    /*template <typename Ret>
    auto send_request_to_json(const auto &r, std::string_view reqname) const {
        nlohmann::json j;
        for_each_field(r, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            to_json(j, get_name<I, std::decay_t<decltype(r)>>(), field);
        });
        j = send_request(reqname, j.dump());
        return from_json<Ret>(j);
    }
    template <typename Ret>
    auto send_request_to_arguments(const auto &r, std::string_view reqname) const {
        std::array<http_request_argument, tuple_size<std::decay_t<decltype(r)>>::value> args;
        auto i = args.begin();
        for_each_field(r, [&]<auto I>(auto &&field, std::integral_constant<size_t, I>) {
            to_request_argument(i, get_name<I, std::decay_t<decltype(r)>>(), field);
        });
        auto j = send_request(reqname, http_request_arguments{args.begin(), i});
        return from_json<Ret>(j);
    }*/

#include <methods.inl.h>

private:
    const Bot &bot;

    // Bot's HttpClient must implement
    //std::string make_request(const std::string &url, const http_request_arguments &args) const;
    //std::string make_request(const std::string &url, const std::string &json) const;
    nlohmann::json send_request(std::string_view method, auto &&args) const {
        auto url = bot.base_url();
        url += bot.token();
        url += "/";
        url += method;

        auto response = bot.http_client().make_request(url, args);
        if (!response.compare(0, 6, "<html>"))
            throw std::runtime_error("tgbot library has got html page instead of json response. Maybe you entered wrong bot token.");

        auto result = nlohmann::json::parse(response);
        if (result["ok"] == true)
            return std::move(result["result"]);
        else
            throw std::runtime_error(result["description"].template get<std::string>());
    }

    template<typename, template <typename...> typename>
    struct is_instance : std::false_type {};
    template<template <typename...> typename C, typename ... Args>
    struct is_instance<C<Args...>, C> : std::true_type {};

    template <typename T>
    static Optional<http_request_argument> to_request_argument(auto &&n, const T &r) {
        if constexpr (is_instance<T, std::vector>::value) {
            if (r.empty())
                return {};
            http_request_argument a;
            a.name = n;
            a.value = to_json(r).dump();
            return a;
        } else if constexpr (is_instance<T, std::unique_ptr>::value || is_instance<T, std::optional>::value) {
            if (r)
                return to_request_argument(n, *r);
            return {};
        } else if constexpr (is_instance<T, std::variant>::value) {
            return std::visit([&n](auto &&r) { return to_request_argument(n, r); }, r);
        } else {
            http_request_argument a;
            a.name = n;
            if constexpr (std::is_same_v<T, Boolean> || std::is_same_v<T, Integer> || std::is_same_v<T, Float>) {
                a.value = std::to_string(r);
            } else if constexpr (std::is_same_v<T, String>) {
                a.value = r;
            } else if constexpr (std::is_same_v<T, InputFile>) {
                a.filename = r.file_name;
                a.mimetype = r.mime_type;
            } else {
                a.value = to_json(r).dump();
            }
            return a;
        }
    }
    static void to_request_argument(auto &&arg, std::string_view n, auto &&r) {
        if (auto v = to_request_argument(n, r); v)
            *arg++ = std::move(*v);
    }

    template <typename T>
    static nlohmann::json to_json(const T &v) {
        if constexpr (is_instance<T, std::vector>::value) {
            if (v.empty())
                return nullptr;
            nlohmann::json j;
            for (auto &v : v)
                j.push_back(to_json(v));
            return j;
        } else if constexpr (is_instance<T, std::unique_ptr>::value || is_instance<T, std::optional>::value) {
            if (!v)
                return nullptr;
            return to_json(*v);
        } else if constexpr (is_instance<T, std::variant>::value) {
            return std::visit([](auto &&v) { return to_json(v); }, v);
        } else if constexpr (is_simple_type<T>) {
            return v;
        } else if constexpr (requires { to_json(v, type<T>{}); }) {
            nlohmann::json j;
            to_json(j, type<T>{});
            return j;
        } else {
            nlohmann::json j;
            for_each_field(v, [&](auto &&field, std::string_view name) {
                to_json(j, name, field);
            });
            return j;
        }
    }
    template <typename T>
    static void to_json(nlohmann::json &j, std::string_view k, const T &r) {
        if (auto v = to_json(r); !v.is_null())
            j[k] = v;
    }

    template <typename T>
    static T from_json(const nlohmann::json &j) {
        if constexpr (is_instance<T, std::vector>::value) {
            Vector<typename T::value_type> v;
            for (auto &i : j)
                v.emplace_back(from_json<typename T::value_type>(i));
            return v;
        } else if constexpr (is_instance<T, std::unique_ptr>::value) {
            auto p = create_ptr<typename T::element_type>();
            *p = from_json<typename T::element_type>(j);
            return p;
        } else if constexpr (is_instance<T, std::optional>::value) {
            return from_json<typename T::value_type>(j);
        } else if constexpr (is_instance<T, std::variant>::value) {
            throw std::logic_error{"no implemented"};
            //return std::visit([&](auto &&r) { return from_json<std::decay_t<decltype(r)>>(r); }, j);
        } else if constexpr (is_simple_type<T>) {
            return j;
        } else if constexpr (requires { refl<T>::is_received_variant; requires refl<T>::is_received_variant; }) {
            return from_json_variant(j, type<T>{});
        } else if constexpr (requires { from_json(j, type<T>{}); }) {
            return from_json(j, type<T>{});
        } else {
            T v;
            for_each_field(v, [&](auto &&field, std::string_view name) {
                from_json(j, name, field);
            });
            return v;
        }
    }
    template <typename T>
    static void from_json(const nlohmann::json &j, std::string_view k, T &v) {
        if (j.contains(k))
            v = from_json<T>(j[k]);
    }
};

/// This object holds other objects specific for this bot instance.
template <typename HttpClient>
struct bot {
    bot(const std::string &token, const HttpClient &client) : token_{token}, http_client_{client} {}

    /// returns token for accessing api
    const std::string &token() const { return token_; }

    /// returns object which can execute Telegram Bot API methods
    const auto &api() const { return api_; }

    /// used for fine tune setup of http connections
    const auto &http_client() const { return http_client_; }

    const std::string &base_url() const { return base_url_; }
    const std::string &base_file_url() const { return base_file_url_; }
    std::string make_file_url(const std::string &file_path) const { return base_file_url_ + token_ + "/" + file_path; }

private:
    std::string token_;
    const HttpClient &http_client_;
    ::tgbot::api<bot<HttpClient>> api_{*this};
    std::string base_url_{ "https://api.telegram.org/bot" };
    std::string base_file_url_{ "https://api.telegram.org/file/bot" };
};

}
