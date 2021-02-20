#pragma once

#include "types.h"

namespace tgbot {

struct bot;

/// This class executes telegram api methods.
/// Telegram docs: <https://core.telegram.org/bots/api#available-methods>
struct TGBOT_API api {
    api(const bot &b) : b(b) {}

#include <methods.inl.h>

private:
    const bot &b;
};

}
