#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace tgbot {

using Boolean = bool;
using Integer = std::int64_t;
using Float = double;
using String = std::string;

template <class T>
using Optional = std::optional<T>;
template <class ... Args>
using Ptr = std::unique_ptr<Args...>;
template <class ... Args>
using Variant = std::variant<Args...>;
template <class ... Args>
using Vector = std::vector<Args...>;

template <class ... PtrArgs, class ... Args>
auto createPtr(Args && ... args) {
    return std::make_unique<PtrArgs...>(args...);
}

#define TGBOT_TYPE_API TGBOT_API
#include <types.inl.h>

} // namespace tgbot
