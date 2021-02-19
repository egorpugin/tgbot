#include <tgbot/bot.h>

#include <tgbot/curl_http_client.h>

#include <memory>
#include <string>

namespace tgbot {

bot::bot(const std::string &token)
    : token(token)
    , httpClient(std::make_unique<curl_http_client>())
    , api(*this)
{
    base_url = "https://api.telegram.org/bot";
    base_file_url = "https://api.telegram.org/file/bot";

    httpClient->set_timeout(get_default_timeout()); // default timeout for apis
}

bot::~bot() {
}

Integer bot::process_updates(Integer offset, Integer limit, Integer timeout, const Vector<String> &allowed_updates) {
    // update timeout here for getUpdates()
    httpClient->set_timeout(timeout);

    auto updates = api.getUpdates(offset, limit, timeout, allowed_updates);
    for (const auto &item : updates) {
        // if updates come unsorted, we must check this
        if (item->update_id >= offset)
            offset = item->update_id + 1;
        try {
            handle_update(*item);
        } catch (std::exception &e) {
            printf("error: %s\n", e.what());
        }
    }
    return offset;
}

void bot::long_poll(Integer limit, Integer timeout, const Vector<String> &allowed_updates) {
    Integer offset = 0;
    while (1)
        offset = process_updates(offset, limit, timeout, allowed_updates);
}

std::string bot::make_file_url(const std::string &file_path) const {
    return base_file_url + get_token() + "/" + file_path;
}

}
