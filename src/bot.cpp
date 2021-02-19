#include <tgbot/bot.h>

#include <tgbot/curl_http_client.h>

#include <memory>
#include <string>

namespace tgbot {

bot::bot(const std::string &token)
    : token_(token)
    , http_client_(std::make_unique<curl_http_client>())
    , api_(*this)
{
    base_url_ = "https://api.telegram.org/bot";
    base_file_url_ = "https://api.telegram.org/file/bot";

    http_client_->set_timeout(default_timeout()); // default timeout for apis
}

bot::~bot() {
}

std::string bot::make_file_url(const std::string &file_path) const {
    return base_file_url_ + token_ + "/" + file_path;
}

}
