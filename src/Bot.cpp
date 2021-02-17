#include "tgbot/Bot.h"

#include "tgbot/CurlHttpClient.h"

#include <memory>
#include <string>

namespace tgbot {

Bot::Bot(const std::string &token)
    : token(token)
    , httpClient(std::make_unique<CurlHttpClient>())
    , api(*this)
{
    base_url = "https://api.telegram.org/bot";
    base_file_url = "https://api.telegram.org/file/bot";

    httpClient->setTimeout(getDefaultTimeout()); // default timeout for apis
}

Bot::~Bot() {
}

Integer Bot::processUpdates(Integer offset, Integer limit, Integer timeout, const Vector<String> &allowed_updates) {
    // update timeout here for getUpdates()
    httpClient->setTimeout(timeout);

    auto updates = api.getUpdates(offset, limit, timeout, allowed_updates);
    for (const auto &item : updates) {
        // if updates come unsorted, we must check this
        if (item->update_id >= offset)
            offset = item->update_id + 1;
        try {
            handleUpdate(*item);
        } catch (std::exception &e) {
            printf("error: %s\n", e.what());
        }
    }
    return offset;
}

void Bot::longPoll(Integer limit, Integer timeout, const Vector<String> &allowed_updates) {
    Integer offset = 0;
    while (1)
        offset = processUpdates(offset, limit, timeout, allowed_updates);
}

std::string Bot::makeFileUrl(const std::string &file_path) const {
    return base_file_url + getToken() + "/" + file_path;
}

}
