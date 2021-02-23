#include <tgbot/curl_http_client.h>

#include <boost/asio.hpp>
#include <boost/asio/experimental/as_single.hpp>

#include <thread>

using namespace boost::asio::ip;

namespace tgbot {

// Die if we get a bad CURLMcode somewhere
static void mcode_or_die(const char *where, CURLMcode code) {
    if (code == CURLM_OK)
        return;
    const char *s;
    switch (code) {
    case CURLM_CALL_MULTI_PERFORM:
        s = "CURLM_CALL_MULTI_PERFORM";
        break;
    case CURLM_BAD_HANDLE:
        s = "CURLM_BAD_HANDLE";
        break;
    case CURLM_BAD_EASY_HANDLE:
        s = "CURLM_BAD_EASY_HANDLE";
        break;
    case CURLM_OUT_OF_MEMORY:
        s = "CURLM_OUT_OF_MEMORY";
        break;
    case CURLM_INTERNAL_ERROR:
        s = "CURLM_INTERNAL_ERROR";
        break;
    case CURLM_UNKNOWN_OPTION:
        s = "CURLM_UNKNOWN_OPTION";
        break;
    case CURLM_LAST:
        s = "CURLM_LAST";
        break;
    default:
        s = "CURLM_unknown";
        break;
    case CURLM_BAD_SOCKET:
        s = "CURLM_BAD_SOCKET";
        fprintf(stderr, "\nERROR: %s returns %s", where, s);
        /* ignore this error */
        return;
    }

    fprintf(stderr, "\nERROR: %s returns %s", where, s);
    exit(code);
}

// Check for completed transfers, and remove their easy handles
void curl_http_client::check_multi_info() {
    char *eff_url;
    CURLMsg *msg;
    int msgs_left;
    //ConnInfo *conn;
    CURL *easy;
    CURLcode res;

    while ((msg = curl_multi_info_read(multi_handle, &msgs_left))) {
        if (msg->msg == CURLMSG_DONE) {
            easy = msg->easy_handle;
            res = msg->data.result;
            //curl_easy_getinfo(easy, CURLINFO_PRIVATE, &conn);
            //curl_easy_getinfo(easy, CURLINFO_EFFECTIVE_URL, &eff_url);
            curl_multi_remove_handle(multi_handle, easy);
            //free(conn->url);
            curl_easy_cleanup(easy);
            //free(conn);
        }
    }
}

void curl_http_client::timer_cb() {
    auto rc = curl_multi_socket_action(multi_handle, CURL_SOCKET_TIMEOUT, 0, &still_running);
    mcode_or_die("timer_cb: curl_multi_socket_action", rc);
    check_multi_info();
}

boost::asio::awaitable<void> curl_http_client::multi_timer_cb2() {
    co_await timer->async_wait(boost::asio::use_awaitable);
    timer_cb();
}

// Update the event timer after curl_multi library calls
int curl_http_client::multi_timer_cb(CURLM *multi, long timeout_ms, curl_http_client *client) {
    auto &timer = *client->timer;
    // cancel running timer
    timer.cancel();

    if (timeout_ms > 0) {
        // update timer
        timer.expires_from_now(boost::posix_time::millisec(timeout_ms));
        client->awaitables.emplace(
            boost::asio::co_spawn(*client->io_context_, client->multi_timer_cb2(), boost::asio::use_awaitable));
    } else if (timeout_ms == 0) {
        // call timeout function immediately
        client->timer_cb();
    }
    return 0;
}

boost::asio::awaitable<void> curl_http_client::event_cb2(boost::asio::ip::tcp::socket &tcp_socket, tcp::socket::wait_type waittype,
    curl_socket_t s, int action, int *fdp) {
    bool ok = false;
    try {
        co_await tcp_socket.async_wait(waittype, boost::asio::experimental::as_single(boost::asio::use_awaitable));
        ok = true;
    } catch (std::exception &) {
    }
    boost::system::error_code ec;
    if (!ok)
        ec = boost::system::errc::make_error_code(boost::system::errc::operation_canceled);
    event_cb(s, action, ec, fdp);
}

// Called by asio when there is an action on a socket
void curl_http_client::event_cb(curl_socket_t s, int action, const boost::system::error_code &error, int *fdp) {
    if (!sockmap.contains(s)) {
        // c-ares or other sockets
        return;
    }

    // make sure the event matches what are wanted
    if (*fdp == action || *fdp == CURL_POLL_INOUT) {
        CURLMcode rc;
        if (error)
            action = CURL_CSELECT_ERR;
        rc = curl_multi_socket_action(multi_handle, s, action, &still_running);

        mcode_or_die("event_cb: curl_multi_socket_action", rc);
        check_multi_info();

        if (still_running <= 0) {
            timer->cancel();
        }

        // keep on watching.
        // the socket may have been closed and/or fdp may have been changed
        // in curl_multi_socket_action(), so check them both
        if (!error && sockmap.contains(s) && (*fdp == action || *fdp == CURL_POLL_INOUT)) {
            auto &tcp_socket = *sockmap.find(s)->second;
            int waittype = -1;
            if (action == CURL_POLL_IN)
                waittype = tcp::socket::wait_read;
            else if (action == CURL_POLL_OUT)
                waittype = tcp::socket::wait_write;
            awaitables.emplace(
                boost::asio::co_spawn(*io_context_,
                    event_cb2(tcp_socket, (tcp::socket::wait_type)waittype, s, action, fdp), boost::asio::use_awaitable));
        }
    }
}

void curl_http_client::addsock(curl_socket_t s, CURL *easy, int action)
{
    // fdp is used to store current action
    int *fdp = (int *)calloc(sizeof(int), 1);
    setsock(fdp, s, easy, action, 0);
    curl_multi_assign(multi_handle, s, fdp);
}

void curl_http_client::remsock(int *f) {
    if (f)
        free(f);
}

void curl_http_client::setsock(int *fdp, curl_socket_t s, CURL *e, int act, int oldact) {
    auto it = sockmap.find(s);
    if (it == sockmap.end()) {
        // c-ares or other sockets
        return;
    }

    int waittype = -1;
    int action = -1;
    switch (act) {
    case CURL_POLL_IN:
        if (oldact != CURL_POLL_IN && oldact != CURL_POLL_INOUT) {
            waittype = tcp::socket::wait_read;
            action = CURL_POLL_IN;
        }
        break;
    case CURL_POLL_OUT:
        if (oldact != CURL_POLL_OUT && oldact != CURL_POLL_INOUT) {
            waittype = tcp::socket::wait_write;
            action = CURL_POLL_OUT;
        }
        break;
    case CURL_POLL_INOUT:
        if (oldact != CURL_POLL_IN && oldact != CURL_POLL_INOUT) {
            waittype = tcp::socket::wait_read;
            action = CURL_POLL_IN;
        }
        if (oldact != CURL_POLL_OUT && oldact != CURL_POLL_INOUT) {
            waittype = tcp::socket::wait_write;
            action = CURL_POLL_OUT;
        }
        break;
    default:
        break;
    }

    if (waittype != -1 && action != -1) {
        auto &tcp_socket = *it->second;
        *fdp = act;
        awaitables.emplace(
            boost::asio::co_spawn(*io_context_,
                event_cb2(tcp_socket, (tcp::socket::wait_type)waittype, s, action, fdp), boost::asio::use_awaitable));
    }
}

int curl_http_client::sock_cb(CURL *e, curl_socket_t s, int what, curl_http_client *client, void *sockp) {
    int *actionp = (int *)sockp;
    if (what == CURL_POLL_REMOVE) {
        client->remsock(actionp);
    } else {
        if (!actionp) {
            client->addsock(s, e, what);
        } else {
            client->setsock(actionp, s, e, what, *actionp);
        }
    }
    return 0;
}

curl_socket_t curl_http_client::open_socket(curlsocktype purpose, struct curl_sockaddr *address) {
    curl_socket_t sockfd = CURL_SOCKET_BAD;

    // restrict to IPv4
    if (purpose != CURLSOCKTYPE_IPCXN || address->family != AF_INET)
        return sockfd;

    // create a tcp socket object
    auto tcp_socket = std::make_unique<tcp::socket>(*io_context_);
    boost::system::error_code ec;
    tcp_socket->open(boost::asio::ip::tcp::v4(), ec);

    if (ec) {
        return sockfd;
    } else {
        sockfd = tcp_socket->native_handle();
        sockmap.emplace(sockfd, std::move(tcp_socket));
    }
    return sockfd;
}

int curl_http_client::close_socket(curl_socket_t item) {
    auto it = sockmap.find(item);
    if (it != sockmap.end()) {
        sockmap.erase(it);
    }
    return 0;
}

std::size_t curl_write_string(char *ptr, std::size_t size, std::size_t nmemb, void *userdata);

struct mytype {
    boost::asio::this_coro::executor_t e;
    mytype(boost::asio::this_coro::executor_t e) : e(e) {}
    operator boost::asio::this_coro::executor_t() const { return e; }
    auto operator co_await() {
        throw;
    }
};
/*auto operator co_await(mytype t){
    struct awaiter {
        //boost::asio::this_coro::executor_t e;
        //awaiter(boost::asio::this_coro::executor_t e) : e(e) {}
        bool await_ready() const { return false; }
        void await_suspend() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
        //void await_suspend(std::coroutine_handle<> resume_cb) {}
        void await_resume() {
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    };
    return awaiter{};
}*/

template <typename T>
struct mytask {
    using promise_type = mytask;

    boost::asio::this_coro::executor_t e;
    mytask() {}
    mytask(boost::asio::this_coro::executor_t e) : e(e) {}
    operator boost::asio::this_coro::executor_t() const { return e; }

    auto initial_suspend() { return std::suspend_always{}; }
    auto final_suspend() { return std::suspend_never{}; }
    void return_void() {}
    mytask<void> get_return_object() { return e; }

    /*auto operator co_await() {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }*/
};

int aaa;

struct mytask2 : mytask<void> {
    using promise_type = mytask2;

    int x = aaa++;

    mytask2() {
        printf("creating mytask2 %p\n", this);
    }
    ~mytask2() {
        printf("destroying mytask2 %p\n", this);
    }
    mytask2 get_return_object() {
        printf("creating mytask2 in get_return_object of %p\n", this);
        return {};
    }

    auto initial_suspend() { return std::suspend_never{}; }

    bool await_ready() const noexcept
    {
        return true;
    }

    // Support for co_await keyword.
    void await_suspend(std::coroutine_handle<mytask2> h)
    {
        printf("await_suspend in %p, passed handle is %p\n", this, h.address());
        auto &p = h.promise();
        int a = 5;
        a++;
        return;
    }

    // Support for co_await keyword.
    auto await_resume()
    {
        return true;
    }
};

mytask2 a() {
    co_return;
}

mytask2 g() {
    co_await a();
}

mytask2 f() {
    co_await g();
    int a = 5;
    a++;
    co_await g();
    a = 5;
    a++;
    co_await g();
}

mytask<void> mycoro() {
    co_return;
}

boost::asio::awaitable<std::string> curl_http_client::execute_async(CURL *curl) const {
    // https://curl.se/libcurl/c/asiohiper.html
    // https://curl.se/libcurl/c/multi-app.html
    // https://curl.se/libcurl/c/multi-single.html

    co_await f();

    if (!async())
        throw std::logic_error("Not in async mode. Call set_io_context() to switch to async mode.");

    //curl_easy_setopt(curl, CURLOPT_PRIVATE, this);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETDATA, this);
    curl_easy_setopt(curl, CURLOPT_OPENSOCKETFUNCTION, &curl_http_client::open_socket);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETDATA, this);
    curl_easy_setopt(curl, CURLOPT_CLOSESOCKETFUNCTION, &curl_http_client::close_socket);

    std::string response;
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_string);

    /*long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code >= 500 || res) {
        std::this_thread::sleep_for(std::chrono::seconds(net_delay_on_error));
        if (net_delay_on_error < 30)
            net_delay_on_error *= 2;
    }
    if (res != CURLE_OK)
        throw std::runtime_error(std::string("curl error: ") + curl_easy_strerror(res));*/
    //return response;

    curl_multi_add_handle(multi_handle, curl);
    //co_await boost::asio::co_spawn(*io_context_, execute_async(curl), boost::asio::use_awaitable);
    //co_return execute(curl);
    //co_await boost::asio::awaitable<std::string>{};
    //co_return "{}";

    while (!awaitables.empty()) {
        auto a = std::move(awaitables.front());
        awaitables.pop();
        co_await std::move(a);
    }

    boost::asio::deadline_timer timer(*io_context_);
    timer.expires_from_now(boost::posix_time::millisec(1000 * 500));
    co_await timer.async_wait(boost::asio::use_awaitable);
    co_await mytype{boost::asio::this_coro::executor};
    co_await mycoro();
    co_return response;
}

void curl_http_client::set_io_context(boost::asio::io_context &io_context) {
    io_context_ = &io_context;
    timer = std::make_unique<boost::asio::deadline_timer>(io_context);
    curl_multi_cleanup(multi_handle);
    multi_handle = curl_multi_init();

    curl_multi_setopt(multi_handle, CURLMOPT_SOCKETFUNCTION, &curl_http_client::sock_cb);
    curl_multi_setopt(multi_handle, CURLMOPT_SOCKETDATA, this);
    curl_multi_setopt(multi_handle, CURLMOPT_TIMERFUNCTION, &curl_http_client::multi_timer_cb);
    curl_multi_setopt(multi_handle, CURLMOPT_TIMERDATA, this);
}

}
