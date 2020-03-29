#include "tgbot/CurlHttpClient.h"

#include "tgbot/HttpRequestArgument.h"

#include <curl/curl.h>

#include <stdexcept>

template <class F>
struct ScopeExit
{
    F f;

    ScopeExit(F f) : f(f) {}
    ~ScopeExit() { f(); }
};

static std::size_t curlWriteString(char *ptr, std::size_t size, std::size_t nmemb, void *userdata)
{
    static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

static CURL *setupConnection(CURL *in, const std::string &url)
{
    // Copy settings for each call because we change CURLOPT_URL and other stuff.
    // This also protects multithreaded case.
    auto curl = curl_easy_duphandle(in);
    //auto curl = in;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    return curl;
}

static std::string execute(CURL *curl)
{
    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteString);

    auto res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));

    return response;
}

namespace TgBot
{

CurlHttpClient::CurlHttpClient()
{
    curlSettings = curl_easy_init();

    // default options
    curl_easy_setopt(getCurl(), CURLOPT_TCP_NODELAY, 1);
}

CurlHttpClient::~CurlHttpClient()
{
    curl_easy_cleanup(curlSettings);
}

std::string CurlHttpClient::makeRequest(const std::string &url, const std::string &json) const
{
    auto curl = setupConnection(getCurl(), url);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    ScopeExit se([headers] { curl_slist_free_all(headers); });

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

    return execute(curl);
}

std::string CurlHttpClient::makeRequest(const std::string &url, const HttpRequestArguments &args) const
{
    auto curl = setupConnection(getCurl(), url);

    curl_mime *mime = nullptr;
    if (!args.empty())
    {
        mime = curl_mime_init(curl);
        for (auto &a : args)
        {
            auto part = curl_mime_addpart(mime);
            curl_mime_name(part, a.name.c_str());
            if (a.isFile)
            {
                auto fn = a.fileName;
                fn = fn.substr(fn.find_last_of("/\\") + 1);
                curl_mime_filename(part, fn.c_str());
                curl_mime_filedata(part, a.fileName.c_str());
                curl_mime_type(part, a.mimeType.c_str());
            }
            else
            {
                curl_mime_data(part, a.value.c_str(), a.value.size());
            }
        }
        curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    }
    ScopeExit se([mime] { curl_mime_free(mime); });

    return execute(curl);
}

}
