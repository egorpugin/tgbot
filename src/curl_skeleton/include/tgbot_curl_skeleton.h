#pragma once

#include "curl_http_client.h"

#include <primitives/http.h>
#include <primitives/sw/main.h>
#include <primitives/sw/settings.h>
#include <primitives/templates2/overload.h>

#include <concepts>
#include <coroutine>
#include <format>
#include <fstream>
#ifdef _WIN32
#include <print>
#endif

using tg_bot_curl = tgbot::bot<curl_http_client>;

decltype(auto) visit(auto &var, auto &&...f) {
    return ::std::visit(overload{FWD(f)...}, var);
}

template <typename... Types>
struct types {
    template <typename... TypesPre>
    using variant_type = std::variant<TypesPre..., Types...>;

    void operator()(auto &&f) const {
        (f(Types{}), ...);
    }
    static void f(auto &&f) {
        (f((Types **)nullptr), ...);
    }
};

struct coro_command_base {
    template <typename T = int>
    struct task : std::coroutine_handle<> {
        struct promise_type {
            coro_command_base *c{};
            std::coroutine_handle<> prev{};
            T retval{};

            task get_return_object() {
                return {std::coroutine_handle<promise_type>::from_promise(*this)};
            }
            std::suspend_always initial_suspend() noexcept {
                return {};
            }
            auto final_suspend() noexcept {
                struct aw {
                    promise_type &p;
                    bool await_ready() noexcept {
                        return false;
                    }
                    std::coroutine_handle<> await_suspend(std::coroutine_handle<>) noexcept {
                        if (p.prev) {
                            p.c->last_coro = p.prev;
                            return p.prev;
                        }
                        return nullptr;
                    }
                    void await_resume() noexcept {
                    }
                };
                return aw{*this};
            }
            /*void return_void() {
                int a = 5;
                a++;
            }*/
            void return_value(auto &&ret) {
                retval = std::move(ret);
            }
            void unhandled_exception() {
                try {
                    tgbot::sendMessageRequest req;
                    req.chat_id = c->chat_id;
                    req.text = "internal error";
                    c->bot->api().sendMessage(req);
                } catch (std::exception &) {
                }
                throw;
            }
            template <typename T2>
            auto await_transform(task<T2> &&t) {
                t.h.promise().c = c;
                t.h.promise().prev = c->last_coro;
                c->last_coro = t.h;
                return std::move(t);
            }
            auto await_transform(auto &&t) {
                return std::move(t);
            }
        };

        std::coroutine_handle<promise_type> h{};

        task() = default;
        task(std::coroutine_handle<promise_type> h) : h{h} {
            //std::println("created {}", h.address());
        }
        task(const task &) = delete;
        task &operator=(const task &) = delete;
        task(task &&rhs) {
            operator=(std::move(rhs));
        }
        task &operator=(task &&rhs) {
            this->~task();
            h = std::move(rhs.h);
            rhs.h = nullptr;
            return *this;
        }
        ~task() {
            if (h && h.done()) {
                //std::println("destroy {}", h.address());
                h.destroy();
            }
        }

        bool await_ready() {
            return !h || h.done();
        }
        std::coroutine_handle<> await_suspend(std::coroutine_handle<>) {
            if (h && h.promise().c->last_coro) {
                return h.promise().c->last_coro;
            }
            return nullptr;
        }
        T await_resume() {
            if constexpr (!std::same_as<T, void>) {
                return h.promise().retval;
            } else {
            }
        }
    };
    struct aw {
        coro_command_base &c;
        bool await_ready() {
            return false;
        }
        void await_suspend(std::coroutine_handle<> h) {
        }
        const tgbot::Message &await_resume() {
            return *c.m;
        }
    };

    tg_bot_curl *bot{};
    task<> top_coro;
    std::coroutine_handle<> last_coro{};
    tgbot::Integer chat_id;
    const tgbot::Message *m;

    // create or coro function
    // returns true if coroutine/coro stack ended
    bool coro(this auto &&cmd, auto &bot, auto &u) {
        cmd.bot = &bot;
        cmd.chat_id = u.id;
        cmd.top_coro = std::move(cmd.coro2(bot, u));
        cmd.top_coro.h.promise().c = &cmd;
        cmd.last_coro = cmd.top_coro.h;
        return cmd.coro_resume(); // we resume top coro manually
    }
    // continue functions
    // returns true if coroutine/coro stack ended
    bool coro(auto &bot, const tgbot::CallbackQuery &q) {
        tgbot::Message mm;
        mm.text = q.data;
        m = &mm;
        return coro_resume();
    }
    bool coro(auto &bot, const tgbot::Message &message) {
        m = &message;
        return coro_resume();
    }
    bool coro_resume() {
        if (!last_coro) {
            return true;
        }
        auto &h = last_coro;
        if (!h || h.done()) {
        } else {
            h.resume();
        }
        return coro_done();
    }
    bool coro_done() const {
        return !last_coro || last_coro.done();
    }

    // helpers
    auto send_message(const tgbot::sendMessageRequest &req) {
        bot->api().sendMessage(req);
        return aw{*this};
    }
    auto send_message(const std::string &text) {
        tgbot::sendMessageRequest req;
        req.chat_id = chat_id;
        req.text = text;
        return send_message(req);
    }

    // useful coros
    task<std::string> get_text(const std::string &message) {
        tgbot::sendMessageRequest req;
        req.chat_id = chat_id;
        req.text = message;
        co_return co_await get_text(req);
    }
    task<std::string> get_text(const tgbot::sendMessageRequest &req) {
        auto msg = &co_await send_message(req);
        while (!msg->text) {
            msg = &co_await send_message(req);
        }
        co_return *msg->text;
    }
    task<int> get_integer(const std::string &message, auto && ... args) {
        tgbot::sendMessageRequest req;
        req.chat_id = chat_id;
        req.text = message;
        co_return co_await get_integer(req, args...);
    }
    task<int> get_integer(const tgbot::sendMessageRequest &req, auto && ... args) {
        co_return co_await get_number<int>(req, [](auto &&v){return std::stoi(v);}, args...);
    }
    task<double> get_float(const std::string &message, auto &&...args) {
        tgbot::sendMessageRequest req;
        req.chat_id = chat_id;
        req.text = message;
        co_return co_await get_float(req, args...);
    }
    task<double> get_float(const tgbot::sendMessageRequest &req, auto &&...args) {
        co_return co_await get_number<double>(req, [](auto &&v){return std::stod(v);}, args...);
    }
    template <typename T>
    task<T> get_number(const tgbot::sendMessageRequest &req, auto &&f,
        T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max()) {
        while (1) {
            auto msg = &co_await send_message(req);
            try {
                auto v = f(*msg->text);
                if (v < min || v > max) {
                    send_message(std::format("bad number: must be between in range [{}, {}]", min, max));
                    continue;
                }
                co_return v;
            } catch (std::exception &) {
                send_message(std::format("bad number: must be between in range [{}, {}]", min, max));
            }
        }
    }
};

struct tg_bot : tg_bot_curl {
    using base = tg_bot_curl;

    std::string botname;
    std::string botvisiblename;
    tgbot::Integer my_id{};

    static const int default_update_limit = 100;
    static const int default_update_timeout = 10;
    static const int default_net_delay_on_error = 1;

public:
    using base::base;

    void init(this auto &&bot) {
        auto me = bot.api().getMe();
        if (!me.username)
            throw SW_RUNTIME_ERROR("Empty bot name");
        bot.botname = *me.username;
        bot.botvisiblename = me.first_name;
        bot.my_id = me.id;
        printf("bot username: %s (%s)\n", me.username->c_str(), me.first_name.c_str());
        // not yet
        //std::printf("bot username: {} ({})", me.username->c_str(), me.first_name.c_str());

        using T = std::decay_t<decltype(bot)>;
        if constexpr (requires { typename T::command_list; }) {
            tgbot::setMyCommandsRequest req;
            req.scope = tgbot::BotCommandScopeAllPrivateChats{"all_private_chats"};
            typename T::command_list{}([&](auto &&c) {
                if constexpr (requires { c.hidden; }) {
                    if (c.hidden) {
                        return;
                    }
                }
                std::string desc;
                if constexpr (requires {c.description;}) {
                    desc = c.description;
                }
                req.commands.push_back(tgbot::BotCommand{c.name, desc});
            });
            bot.api().setMyCommands(req);
        }
    }
    void long_poll(this auto &&bot, tgbot::Integer limit = default_update_limit, tgbot::Integer timeout = default_update_timeout,
                   const tgbot::Optional<tgbot::Vector<String>> &allowed_updates = {}) {
        tgbot::Integer offset{};
        while (1) {
            offset = bot.process_updates(offset, limit, timeout, allowed_updates);
        }
    }
    tgbot::Integer process_updates(this auto &&bot, tgbot::Integer offset = 0, tgbot::Integer limit = default_update_limit,
                                   tgbot::Integer timeout = default_update_timeout,
                                   const tgbot::Optional<tgbot::Vector<String>> &allowed_updates = {}) {
        // update timeout here for getUpdates()
        ((curl_http_client &)bot.http_client()).set_timeout(timeout);

        int net_delay_on_error = default_net_delay_on_error;
        auto updates = bot.api().getUpdates(offset, limit, timeout, allowed_updates);
        for (auto &&update : updates) {
            if (update.update_id >= offset) {
                offset = update.update_id + 1;
            }
            try {
                bot.handle_update(std::move(update));

                net_delay_on_error /= 2;
                if (net_delay_on_error == 0) {
                    net_delay_on_error = default_net_delay_on_error;
                }
            } catch (std::exception &e) {
                printf("error: %s\n", e.what());

                std::this_thread::sleep_for(std::chrono::seconds(net_delay_on_error));
                if (net_delay_on_error < 30)
                    net_delay_on_error *= 2;
            }
        }
        return offset;
    }

public:
    std::string get_available_commands(this auto &&bot) {
        using T = std::decay_t<decltype(bot)>;

        auto text = "Available commands:\n"s;
        typename T::command_list{}([&](auto &&c) {
            text += std::format("/{} {}\n", c.name, c.description);
        });
        return text;
    }
};

template <typename Bot>
int main(int argc, char *argv[]) {
    primitives::http::setupSafeTls();

    sw::setting<std::string> bot_token("bot_token");
    sw::setting<std::string> proxy_host("proxy_host");
    sw::setting<std::string> proxy_user("proxy_user");

    // read
    sw::getSettings(sw::SettingsType::Local);

    std::string token = bot_token;
    if (auto t = getenv("BOT_TOKEN")) {
        token = t;
    }

    curl_http_client client;
    // setup proxy
    {
        auto curl = client.curl_settings();
        if (!proxy_host.getValue().empty()) {
            curl_easy_setopt(curl, CURLOPT_PROXY, proxy_host.getValue().c_str());
            curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
            if (proxy_host.getValue().find("socks5") == 0) {
                curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
                curl_easy_setopt(curl, CURLOPT_SOCKS5_AUTH, CURLAUTH_BASIC);
                curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_BASIC);
            }
            if (!proxy_user.getValue().empty())
                curl_easy_setopt(curl, CURLOPT_PROXYUSERPWD, proxy_user.getValue().c_str());
        }
    }

    auto bot = std::make_unique<Bot>(token, client);
    bot->init();
    bot->long_poll();

    return 0;
}
