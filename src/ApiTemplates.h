template <class T>
static void fromJson(const nlohmann::json &j, T &v);

template <template <typename> typename C, typename T>
static void fromJson(const nlohmann::json &j, C<T> &v);

template <class T>
static void fromJson(const nlohmann::json &j, Vector<T> &v);

//

template <class T>
static void fromJson(const nlohmann::json &j, T &v)
{
    v = j;
}

template <template <typename> typename C, typename T>
static void fromJson(const nlohmann::json &j, C<T> &v)
{
    if constexpr (std::is_same_v<C<T>, Optional<T>>)
    {
        T t;
        fromJson(j, t);
        v = std::move(t);
    }
    else if constexpr (std::is_same_v<C<T>, Ptr<T>>)
    {
        v = createPtr<T>();
        fromJson(j, *v);
    }
    else
        static_assert(false);
}

template <class T>
static void fromJson(const nlohmann::json &j, Vector<T> &v)
{
    for (auto &i : j)
    {
        T t;
        fromJson(i, t);
        v.emplace_back(std::move(t));
    }
}

// must provide decls first

template <class T>
static nlohmann::json toJson(const T &r);

template <class T>
static nlohmann::json toJson(const Optional<T> &r);

template <class T>
static nlohmann::json toJson(const Ptr<T> &r);

template <class T>
static nlohmann::json toJson(const Vector<T> &r);

template <class ... Args>
static nlohmann::json toJson(const Variant<Args...> &r);

//

template <class T>
static nlohmann::json toJson(const T &r)
{
    return r;
}

template <class T>
static nlohmann::json toJson(const Optional<T> &r)
{
    if (!r)
        return {};
    return toJson(*r);
}

template <class T>
static nlohmann::json toJson(const Ptr<T> &r)
{
    if (!r)
        return {};
    return toJson(*r);
}

template <class T>
static nlohmann::json toJson(const Vector<T> &r)
{
    nlohmann::json j;
    for (auto &v : r)
        j.push_back(toJson(v));
    return j;
}

template <class ... Args>
static nlohmann::json toJson(const Variant<Args...> &r)
{
    return std::visit([](auto &&r) { return toJson(r); }, r);
}

// must provide decls first

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const T &r);

template <template <typename> typename C, typename T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const C<T> &r);

template <class ... Args>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Variant<Args...> &r);

//

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const T &r)
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

template <template <typename> typename C, typename T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const C<T> &r)
{
    if constexpr (std::is_same_v<C<T>, Vector<T>>)
    {
        if (r.empty())
            return {};
        HttpRequestArgument a;
        a.name = n;
        a.value = toJson(r).dump();
        return a;
    }
    else
    {
        if (r)
            return toRequestArgument(n, *r);
        return {};
    }
}

template <class ... Args>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Variant<Args...> &r)
{
    return std::visit([&n](auto &&r) { return toRequestArgument(n, r); }, r);
}
