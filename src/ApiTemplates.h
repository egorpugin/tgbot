template<typename, template <typename...> typename>
struct is_instance : std::false_type {};
template<template <typename...> typename C, typename ... Args>
struct is_instance<C<Args...>, C> : std::true_type {};

//

template <class T>
static T from_json(const nlohmann::json &j) {
    if constexpr (is_instance<T, std::vector>::value) {
        Vector<T::value_type> v;
        for (auto &i : j)
            v.emplace_back(from_json<T::value_type>(i));
        return v;
    } else if constexpr (is_instance<T, std::unique_ptr>::value) {
        auto p = createPtr<T::element_type>();
        *p = from_json<T::element_type>(j);
        return p;
    } else if constexpr (is_instance<T, std::optional>::value) {
        return from_json<T::value_type>(j);
    } else {
        return j;
    }
}

template <class T>
static nlohmann::json to_json(const T &r) {
    if constexpr (is_instance<T, std::vector>::value) {
        nlohmann::json j;
        for (auto &v : r)
            j.push_back(to_json(v));
        return j;
    } else if constexpr (is_instance<T, std::unique_ptr>::value || is_instance<T, std::optional>::value) {
        if (!r)
            return {};
        return to_json(*r);
    } else if constexpr (is_instance<T, std::variant>::value) {
        return std::visit([](auto &&r) { return to_json(r); }, r);
    } else {
        return r;
    }
}

template <class T>
static Optional<HttpRequestArgument> to_request_argument(const String &n, const T &r) {
    if constexpr (is_instance<T, std::vector>::value) {
        if (r.empty())
            return {};
        HttpRequestArgument a;
        a.name = n;
        a.value = to_json(r).dump();
        return a;
    } else if constexpr (is_instance<T, std::unique_ptr>::value || is_instance<T, std::optional>::value) {
        if (r)
            return to_request_argument(n, *r);
        return {};
    } else if constexpr (is_instance<T, std::variant>::value) {
        return std::visit([&n](auto &&r) { return to_request_argument(n, r); }, r);
    } else {
        HttpRequestArgument a;
        a.name = n;
        if constexpr (std::is_same_v<T, Boolean> || std::is_same_v<T, Integer> || std::is_same_v<T, Float>) {
            a.value = std::to_string(r);
        } else if constexpr (std::is_same_v<T, String>) {
            a.value = r;
        } else if constexpr (std::is_same_v<T, InputFile>) {
            a.isFile = true;
            a.fileName = r.file_name;
            a.mimeType = r.mime_type;
        } else {
            a.value = to_json(r).dump();
        }
        return a;
    }
}
