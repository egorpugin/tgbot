#pragma once

#include "tgbot/Types.h"

namespace tgbot {

class bot;

/// This class executes telegram api methods.
/// Telegram docs: <https://core.telegram.org/bots/api#available-methods>
struct TGBOT_API api {
    api(const bot &);

#include <methods.inl.h>

private:
    const bot &b;
};

}
