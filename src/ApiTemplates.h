template<typename, template <typename...> typename>
struct is_instance : std::false_type {};
template<template <typename...> typename C, typename ... Args>
struct is_instance<C<Args...>, C> : std::true_type {};

//

template <class T>
static T fromJson(const nlohmann::json &j)
{
    if constexpr (is_instance<T, std::vector>::value)
    {
        Vector<T::value_type> v;
        for (auto &i : j)
            v.emplace_back(fromJson<T::value_type>(i));
        return v;
    }
    else if constexpr (is_instance<T, std::unique_ptr>::value)
    {
        auto p = createPtr<T::element_type>();
        *p = fromJson<T::element_type>(j);
        return p;
    }
    else if constexpr (is_instance<T, std::optional>::value)
        return fromJson<T::value_type>(j);
    else
        return j;
}

template <class T>
static nlohmann::json toJson(const T &r)
{
    if constexpr (is_instance<T, std::vector>::value)
    {
        nlohmann::json j;
        for (auto &v : r)
            j.push_back(toJson(v));
        return j;
    }
    else if constexpr (is_instance<T, std::unique_ptr>::value || is_instance<T, std::optional>::value)
    {
        if (!r)
            return {};
        return toJson(*r);
    }
    else if constexpr (is_instance<T, std::variant>::value)
    {
        return std::visit([](auto &&r) { return toJson(r); }, r);
    }
    else
        return r;
}

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const T &r)
{
    if constexpr (is_instance<T, std::vector>::value)
    {
        if (r.empty())
            return {};
        HttpRequestArgument a;
        a.name = n;
        a.value = toJson(r).dump();
        return a;
    }
    else if constexpr (is_instance<T, std::unique_ptr>::value || is_instance<T, std::optional>::value)
    {
        if (r)
            return toRequestArgument(n, *r);
        return {};
    }
    else if constexpr (is_instance<T, std::variant>::value)
    {
        return std::visit([&n](auto &&r) { return toRequestArgument(n, r); }, r);
    }
    else
    {
        HttpRequestArgument a;
        a.name = n;
        if constexpr (std::is_same_v<T, Boolean> || std::is_same_v<T, Integer> || std::is_same_v<T, Float>)
            a.value = std::to_string(r);
        else if constexpr (std::is_same_v<T, String>)
            a.value = r;
        else if constexpr (std::is_same_v<T, InputFile>)
        {
            a.isFile = true;
            a.fileName = r.file_name;
            a.mimeType = r.mime_type;
        }
        else
            a.value = toJson(r).dump();
        return a;
    }
}
