#include "tgbot/CurlHttpClient.h"

#include <curl/curl.h>

#include <stdexcept>

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

static std::size_t curlWriteString(char *ptr, std::size_t size, std::size_t nmemb, void *userdata)
{
    static_cast<std::string*>(userdata)->append(ptr, size * nmemb);
    return size * nmemb;
}

std::string CurlHttpClient::makeRequest(const std::string &url, const std::string &json) const
{
    /*
    * If there's no args specified, a GET request will be sent, otherwise a POST request will be sent.
    * If at least 1 arg is marked as file, the content type of a request will be multipart/form-data, otherwise it will be application/x-www-form-urlencoded.
    */

    // TODO: add sending files

    // Copy settings for each call because we change CURLOPT_URL and other stuff.
    // This also protects multithreaded case.
    auto curl = curl_easy_duphandle(getCurl());
    //auto curl = getCurl();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    struct curl_slist* headers = nullptr;
    //headers = curl_slist_append(headers, "Connection: close"); // disable keep-alive, why?
    //headers = curl_slist_append(headers, "Connection: keep-alive");
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json.c_str());

    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteString);

    auto res = curl_easy_perform(curl);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));

    return response;
}

}
