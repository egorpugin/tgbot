#pragma once

#include "tgbot/Types.h"

namespace TgBot
{

class Bot;

/// This class executes telegram api methods.
/// Telegram docs: <https://core.telegram.org/bots/api#available-methods>
class TGBOT_API Api
{
public:
    Api(const Bot &);

#include <methods.inl.h>

private:
    const Bot &bot;
};

}
