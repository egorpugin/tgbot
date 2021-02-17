#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace tgbot
{

namespace this_namespace = ::tgbot;

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

template <class ... Args>
auto createPtr()
{
    return std::make_unique<Args...>();
}

#include <types.inl.h>

}
