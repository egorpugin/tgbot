template<typename, template <typename...> typename>
struct is_instance : std::false_type {};
template<template <typename...> typename C, typename ... Args>
struct is_instance<C<Args...>, C> : std::true_type {};

//

template <typename T>
static T from_json(const nlohmann::json &j) {
    if constexpr (is_instance<T, std::vector>::value) {
        Vector<typename T::value_type> v;
        for (auto &i : j)
            v.emplace_back(from_json<typename T::value_type>(i));
        return v;
    } else if constexpr (is_instance<T, std::unique_ptr>::value) {
        auto p = createPtr<typename T::element_type>();
        *p = from_json<typename T::element_type>(j);
        return p;
    } else if constexpr (is_instance<T, std::optional>::value) {
        return from_json<typename T::value_type>(j);
    } else {
        return j;
    }
}

template <typename T>
static void from_json(const nlohmann::json &j, const char *k, T &v) {
    if (j.contains(k))
        v = from_json<T>(j[k]);
}

//

template <typename T>
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

template <typename T>
static void to_json(nlohmann::json &j, const char *k, const T &r) {
    if (auto v = to_json(r); !v.is_null())
        j[k] = v;
}

//

template <typename T>
static Optional<http_request_argument> to_request_argument(const char *n, const T &r) {
    if constexpr (is_instance<T, std::vector>::value) {
        if (r.empty())
            return {};
        http_request_argument a;
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
        http_request_argument a;
        a.name = n;
        if constexpr (std::is_same_v<T, Boolean> || std::is_same_v<T, Integer> || std::is_same_v<T, Float>) {
            a.value = std::to_string(r);
        } else if constexpr (std::is_same_v<T, String>) {
            a.value = r;
        } else if constexpr (std::is_same_v<T, InputFile>) {
            a.filename = r.file_name;
            a.mimetype = r.mime_type;
        } else {
            a.value = to_json(r).dump();
        }
        return a;
    }
}

template <typename T, typename Args>
static void to_request_argument(Args &args, const char *n, const T &r) {
    if (auto v = to_request_argument(n, r); v)
        args.push_back(std::move(*v));
}
