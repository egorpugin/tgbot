#include <tgbot_curl_skeleton.h>

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

    void handle_update(const tgbot::Update &update) {
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
    return main<tg_report_detection_bot>(argc, argv);
}
