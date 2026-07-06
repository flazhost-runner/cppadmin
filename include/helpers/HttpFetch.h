#pragma once
#include <curl/curl.h>
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

// HTTP GET keluar (outbound) berbasis libcurl, dibungkus awaiter coroutine.
//
// Kenapa bukan drogon::HttpClient? Client drogon butuh trantor ber-TLS;
// pada build drogon tanpa OpenSSL ("SSL enabled: false") request https diam-
// diam gagal (NetworkFailure). libcurl sudah di-link repo ini (CMakeLists:
// find_package(CURL REQUIRED)) dan selalu mendukung https. curl_easy bersifat
// blocking, maka dijalankan di worker thread sekali-pakai; koroutin di-resume
// kembali ke event loop asal via queueInLoop — IO loop tidak pernah terblokir.

struct HttpFetchResult {
    long status{0};      // HTTP status code (0 bila transport error)
    std::string body;    // isi respons
    std::string error;   // non-empty = transport error (timeout, DNS, TLS, …)
};

namespace httpfetch_detail {

inline size_t writeCb(char *ptr, size_t size, size_t nmemb, void *userdata) {
    auto *s = static_cast<std::string *>(userdata);
    s->append(ptr, size * nmemb);
    return size * nmemb;
}

inline HttpFetchResult getBlocking(const std::string &url,
                                   const std::vector<std::string> &headers,
                                   long timeoutSec) {
    static std::once_flag initFlag;
    std::call_once(initFlag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });

    HttpFetchResult res;
    CURL *curl = curl_easy_init();
    if (!curl) {
        res.error = "curl_easy_init gagal";
        return res;
    }
    curl_slist *hdrs = nullptr;
    for (const auto &line : headers) hdrs = curl_slist_append(hdrs, line.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeoutSec);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeCb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &res.body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "CppAdmin");
    curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");  // aktifkan gzip
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);         // aman multi-thread
    if (hdrs) curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hdrs);

    CURLcode rc = curl_easy_perform(curl);
    if (rc != CURLE_OK) {
        res.error = curl_easy_strerror(rc);
    } else {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &res.status);
    }
    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(curl);
    return res;
}

// Awaiter: jalankan getBlocking di worker thread, resume di loop asal.
struct HttpGetAwaiter : public drogon::CallbackAwaiter<HttpFetchResult> {
    HttpGetAwaiter(std::string url, std::vector<std::string> headers, long timeoutSec)
        : url_(std::move(url)), headers_(std::move(headers)), timeoutSec_(timeoutSec) {}

    void await_suspend(std::coroutine_handle<> handle) {
        auto *loop = trantor::EventLoop::getEventLoopOfCurrentThread();
        std::thread([this, handle, loop]() {
            setValue(getBlocking(url_, headers_, timeoutSec_));
            if (loop) {
                loop->queueInLoop([handle]() { handle.resume(); });
            } else {
                handle.resume();
            }
        }).detach();
    }

private:
    std::string url_;
    std::vector<std::string> headers_;
    long timeoutSec_;
};

}  // namespace httpfetch_detail

// co_await httpGetCoro(url, {"Accept: application/json"}, 15)
inline httpfetch_detail::HttpGetAwaiter httpGetCoro(
    std::string url, std::vector<std::string> headers = {}, long timeoutSec = 15) {
    return httpfetch_detail::HttpGetAwaiter(std::move(url), std::move(headers), timeoutSec);
}
