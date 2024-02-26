#include "curl_http_client.h"

#include <primitives/http.h>
#include <primitives/sw/main.h>
#include <primitives/sw/settings.h>

#include <fstream>
#include <iostream>
#include <thread>

struct tg_bot : tgbot::bot<curl_http_client> {
    using base = tgbot::bot<curl_http_client>;

    std::string botname;
    std::string botvisiblename;

    static const int default_update_limit = 100;
    static const int default_update_timeout = 10;
    int net_delay_on_error = 1;

public:
    using base::base;

    void init() {
        auto me = api().getMe();
        if (!me.username)
            throw SW_RUNTIME_ERROR("Empty bot name");
        botname = *me.username;
        botvisiblename = me.first_name;
        printf("bot username: %s (%s)\n", me.username->c_str(), me.first_name.c_str());
    }
    tgbot::Integer process_updates(
        tgbot::Integer offset = 0,
        tgbot::Integer limit = default_update_limit,
        tgbot::Integer timeout = default_update_timeout,
        const tgbot::Optional<tgbot::Vector<String>> &allowed_updates = {}) {
        // update timeout here for getUpdates()
        ((curl_http_client&)http_client()).set_timeout(timeout);

        auto updates = api().getUpdates(offset, limit, timeout, allowed_updates);
        for (const auto &item : updates) {
            // if updates come unsorted, we must check this
            if (item.update_id >= offset)
                offset = item.update_id + 1;
            process_update(item);
        }
        return offset;
    }
    void process_update(const tgbot::Update &update) {
        try {
            handle_update(update);
        }
        catch (std::exception &e) {
            printf("error: %s\n", e.what());

            std::this_thread::sleep_for(std::chrono::seconds(net_delay_on_error));
            if (net_delay_on_error < 30)
                net_delay_on_error *= 2;
        }
    }
    void long_poll(
        tgbot::Integer limit = default_update_limit,
        tgbot::Integer timeout = default_update_timeout,
        const tgbot::Optional<tgbot::Vector<String>> &allowed_updates = {}) {
        tgbot::Integer offset = 0;
        while (1) {
            try {
                offset = process_updates(offset, limit, timeout, allowed_updates);
            } catch (std::exception &e) {
                printf("error: %s\n", e.what());
                ++offset; // we skip erroneous message
            }
        }
    }
    ///
    virtual void handle_update(const tgbot::Update &update) = 0;
};

struct user_data {
};

struct tg_report_detection_bot : tg_bot {
    static inline auto fn = "users.txt";
    std::unordered_map<tgbot::Integer, user_data> users;

    using tg_bot::tg_bot;

    tg_report_detection_bot(auto &&token, auto &&client) : tg_bot{token, client} {
        std::ifstream ifile(fn);
        std::string s;
        while (std::getline(ifile, s)) {
            if (s.empty())
                continue;
            auto id = std::stoull(s.substr(1));
            if (s[0] == '+')
                users.emplace(id, user_data{});
            else
                users.erase(id);
        }
    }
    std::ofstream &file() {
        static std::ofstream of(fn, std::ios::out | std::ios::app);
        return of;
    }
    void add_user(auto id) {
        users.erase(id);
        users.emplace(id, user_data{});
        file() << '+' << id << std::endl;
    }
    void remove_user(auto id) {
        users.erase(id);
        file() << '-' << id << std::endl;
    }

    void handle_update(const tgbot::Update &update) override {
        if (update.message)
            handle_message(*update.message);
    }
    void handle_message(const tgbot::Message &message) {
        if (!message.text || !message.text->starts_with("/"sv))
            return;
        if (message.chat && message.chat->id < 0 && !users.empty()) {
            if (*message.text == "/report"sv) {
                for (auto &&member : api().getChatAdministrators(message.chat->id)) {
                    std::visit([this, &message](auto &&u) {
                        if (!users.contains(u.user->id))
                            return;
                        tgbot::sendMessageRequest r;
                        r.chat_id = u.user->id;
                        r.text = "report is called";
                        if (message.chat->username)
                            r.text += " in chat t.me/" + *message.chat->username + "/" + std::to_string(message.message_id);
                        else
                            r.text += " in private chat";
                        api().sendMessage(r);
                    }, member);
                }
            }
        }
        else {
            handle_command(message);
        }
    }
    void handle_command(const tgbot::Message &message) {
        auto &t = *message.text;
        auto cmd = t.substr(1, t.find_first_of(" @"));
        if (cmd == "start"sv) {
            add_user(message.from->id);
            api().sendMessage(message.from->id, "ok");
        } else if (cmd == "stop"sv) {
            remove_user(message.from->id);
            api().sendMessage(message.from->id, "ok");
        } else if (cmd == "help"sv) {
            auto text =
                "Available commands:\n"
                "/start\n"
                "/stop\n"
                "/help\n"
                ;
            api().sendMessage(message.from->id, text);
        } else {
            auto text = "Command '" + *message.text +  "' is not implemented.";
            api().sendMessage(message.from->id, text);
        }
    }
};

int main(int argc, char *argv[]) {
    primitives::http::setupSafeTls();

    sw::setting<std::string> bot_token("bot_token");
    sw::setting<std::string> proxy_host("proxy_host");
    sw::setting<std::string> proxy_user("proxy_user");

    // read
    sw::getSettings(sw::SettingsType::Local);

    curl_http_client client;
    // setup
    {
        auto curl = client.curl_settings();
        primitives::http::setup_curl_ssl(curl);

        //curl_easy_setopt(curl, CURLOPT_VERBOSE, 1);
        //curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, my_trace);

        //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        //curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2);
    }
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

    auto bot = std::make_unique<tg_report_detection_bot>(bot_token, client);
    bot->init();
    bot->long_poll();

    return 0;
}
