#pragma once

#include "types.h"

#include <boost/asio/awaitable.hpp>

namespace tgbot {

struct bot;

/// This class executes telegram api methods.
/// Telegram docs: <https://core.telegram.org/bots/api#available-methods>
struct TGBOT_API api {
    template <typename ... Args>
    using awaitable = boost::asio::awaitable<Args...>;

    api(const bot &b) : b(b) {}

#include <methods.inl.h>

private:
    const bot &b;
};

}
