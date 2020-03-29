template <class T>
static void fromJson(const nlohmann::json &j, T &v);

template <class T>
static void fromJson(const nlohmann::json &j, Optional<T> &v);

template <class T>
static void fromJson(const nlohmann::json &j, Ptr<T> &v);

template <class T>
static void fromJson(const nlohmann::json &j, Vector<T> &v);

//

template <class T>
static void fromJson(const nlohmann::json &j, T &v)
{
    v = j;
}

template <class T>
static void fromJson(const nlohmann::json &j, Optional<T> &v)
{
    T t;
    fromJson(j, t);
    v = std::move(t);
}

template <class T>
static void fromJson(const nlohmann::json &j, Ptr<T> &v)
{
    v = createPtr<T>();
    fromJson(j, *v);
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

template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const Boolean &r);
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const Integer &r);
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const Float &r);
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const String &r);
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const InputFile &r);

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Optional<T> &r);

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Ptr<T> &r);

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Vector<T> &r);

template <class ... Args>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Variant<Args...> &r);

//

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const T &r)
{
    HttpRequestArgument a;
    a.name = n;
    a.value = toJson(r).dump();
    return a;
}

template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const Boolean &r)
{
    HttpRequestArgument a;
    a.name = n;
    a.value = std::to_string(r);
    return a;
}
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const Integer &r)
{
    HttpRequestArgument a;
    a.name = n;
    a.value = std::to_string(r);
    return a;
}
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const Float &r)
{
    HttpRequestArgument a;
    a.name = n;
    a.value = std::to_string(r);
    return a;
}
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const String &r)
{
    HttpRequestArgument a;
    a.name = n;
    a.value = r;
    return a;
}
template <>
Optional<HttpRequestArgument> toRequestArgument(const String &n, const InputFile &r)
{
    HttpRequestArgument a;
    a.name = n;
    a.isFile = true;
    a.fileName = r.file_name;
    a.mimeType = r.mime_type;
    return a;
}

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Optional<T> &r)
{
    if (r)
        return toRequestArgument(n, *r);
    return {};
}

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Ptr<T> &r)
{
    if (r)
        return toRequestArgument(n, *r);
    return {};
}

template <class T>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Vector<T> &r)
{
    if (r.empty())
        return {};
    HttpRequestArgument a;
    a.name = n;
    a.value = toJson(r).dump();
    return a;
}

template <class ... Args>
static Optional<HttpRequestArgument> toRequestArgument(const String &n, const Variant<Args...> &r)
{
    return std::visit([&n](auto &&r) { return toRequestArgument(n, r); }, r);
}
