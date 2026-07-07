#pragma once
// ---------------------------------------------------------------------------
// Storage: adapter penyimpanan berkas driver-aware (local | oss | s3).
//
// Mirror desain NodeAdmin (src/config/storageClient.ts + src/services/
// fileService.ts): DB/HTML menyimpan *key* objek; URL dibangun saat request.
//
//   • local  → berkas disimpan di STORAGE base dir (storage/uploads) dan
//              disajikan pada prefix URL STABIL "/storage/<key>" (lihat
//              StorageController). URL dipisah dari path filesystem.
//   • oss/s3 → URL presigned absolut ber-TTL; tidak ada penyajian lokal.
//
// Berganti backend cukup ubah STORAGE_DRIVER di .env — tanpa sentuh kode/view.
//
// Kripto (HMAC/SHA) memakai OpenSSL yang sudah ter-link; upload/hapus remote
// memakai libcurl (sudah ter-link, lihat HttpFetch.h) via URL presigned.
// ---------------------------------------------------------------------------
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <trantor/net/EventLoop.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <curl/curl.h>
#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include "../AppConfig.h"

namespace storage {

// Prefix URL statis untuk driver local (dipisah dari path filesystem, seperti
// LOCAL_URL_PREFIX di NodeAdmin). StorageController me-mount baseDir() di sini.
inline constexpr const char *kLocalUrlPrefix = "/storage";

// Direktori absolut penyimpanan lokal = <appRoot>/storage/uploads
// (di-set via setUploadPath di main.cc — konsisten, bebas CWD).
inline std::filesystem::path baseDir() {
    return std::filesystem::path(drogon::app().getUploadPath());
}

inline bool isLocal() { return AppConfig::instance().storageDriver == "local"; }

namespace detail {

inline std::string toHex(const unsigned char *d, size_t n) {
    static const char *hx = "0123456789abcdef";
    std::string o;
    o.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { o += hx[d[i] >> 4]; o += hx[d[i] & 0xf]; }
    return o;
}

inline std::string sha256Hex(const std::string &s) {
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr);
    EVP_DigestUpdate(ctx, s.data(), s.size());
    EVP_DigestFinal_ex(ctx, md, &len);
    EVP_MD_CTX_free(ctx);
    return toHex(md, len);
}

inline std::string hmacRaw(const EVP_MD *md, const std::string &key, const std::string &data) {
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int len = 0;
    HMAC(md, key.data(), (int)key.size(),
         reinterpret_cast<const unsigned char *>(data.data()), data.size(), out, &len);
    return std::string(reinterpret_cast<char *>(out), len);
}
inline std::string hmacSha256Raw(const std::string &k, const std::string &d) { return hmacRaw(EVP_sha256(), k, d); }
inline std::string hmacSha256Hex(const std::string &k, const std::string &d) {
    auto r = hmacSha256Raw(k, d);
    return toHex(reinterpret_cast<const unsigned char *>(r.data()), r.size());
}
inline std::string hmacSha1Raw(const std::string &k, const std::string &d) { return hmacRaw(EVP_sha1(), k, d); }

// base64 standar (untuk signature OSS v1)
inline std::string base64(const std::string &in) {
    static const char *e = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string o;
    const unsigned char *d = reinterpret_cast<const unsigned char *>(in.data());
    size_t n = in.size(), i = 0;
    for (; i + 2 < n; i += 3) {
        unsigned b = (d[i] << 16) | (d[i + 1] << 8) | d[i + 2];
        o += e[(b >> 18) & 63]; o += e[(b >> 12) & 63]; o += e[(b >> 6) & 63]; o += e[b & 63];
    }
    if (i < n) {
        unsigned b = d[i] << 16;
        if (i + 1 < n) b |= d[i + 1] << 8;
        o += e[(b >> 18) & 63];
        o += e[(b >> 12) & 63];
        o += (i + 1 < n) ? e[(b >> 6) & 63] : '=';
        o += '=';
    }
    return o;
}

// RFC3986 percent-encode; encodeSlash=false mempertahankan '/' (untuk path).
inline std::string uriEncode(const std::string &s, bool encodeSlash = true) {
    static const char *hx = "0123456789ABCDEF";
    std::string o;
    o.reserve(s.size());
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            o += (char)c;
        } else if (c == '/' && !encodeSlash) {
            o += '/';
        } else {
            o += '%'; o += hx[c >> 4]; o += hx[c & 0xf];
        }
    }
    return o;
}

inline std::string stripScheme(const std::string &e) {
    auto p = e.find("://");
    return p == std::string::npos ? e : e.substr(p + 3);
}

// ── AWS SigV4 presigned URL (query auth) — port setia dari NodeAdmin
//    storageClient.ts s3PresignedUrl; method GET/PUT/DELETE. ────────────────
inline std::string presignS3(const std::string &method, const std::string &key, int ttl) {
    const auto &c = AppConfig::instance();
    std::string region = c.storageRegion.empty() ? "us-east-1" : c.storageRegion;
    bool pathStyle = !c.storageEndpoint.empty();
    std::string host = pathStyle ? stripScheme(c.storageEndpoint)
                                 : c.storageBucket + ".s3." + region + ".amazonaws.com";
    std::string canonicalUri = pathStyle ? "/" + c.storageBucket + "/" + key : "/" + key;

    std::time_t t = std::time(nullptr);
    std::tm g{};
    gmtime_r(&t, &g);
    char db[16], tb[32];
    strftime(db, sizeof(db), "%Y%m%d", &g);
    strftime(tb, sizeof(tb), "%Y%m%dT%H%M%SZ", &g);
    std::string dateStr = db, dtStr = tb;
    std::string scope = dateStr + "/" + region + "/s3/aws4_request";

    std::vector<std::pair<std::string, std::string>> qp = {
        {"X-Amz-Algorithm", "AWS4-HMAC-SHA256"},
        {"X-Amz-Credential", c.storageAccessKeyId + "/" + scope},
        {"X-Amz-Date", dtStr},
        {"X-Amz-Expires", std::to_string(ttl)},
        {"X-Amz-SignedHeaders", "host"},
    };
    std::sort(qp.begin(), qp.end());
    std::string cqs;
    for (size_t i = 0; i < qp.size(); ++i) {
        if (i) cqs += '&';
        cqs += uriEncode(qp[i].first) + "=" + uriEncode(qp[i].second);
    }

    std::string encodedUri = uriEncode(canonicalUri, false);
    std::string canonicalReq =
        method + "\n" + encodedUri + "\n" + cqs + "\nhost:" + host + "\n\nhost\nUNSIGNED-PAYLOAD";
    std::string reqHash = sha256Hex(canonicalReq);
    std::string sts = "AWS4-HMAC-SHA256\n" + dtStr + "\n" + scope + "\n" + reqHash;

    std::string k1 = hmacSha256Raw("AWS4" + c.storageSecretAccessKey, dateStr);
    std::string k2 = hmacSha256Raw(k1, region);
    std::string k3 = hmacSha256Raw(k2, "s3");
    std::string signingKey = hmacSha256Raw(k3, "aws4_request");
    std::string sig = hmacSha256Hex(signingKey, sts);

    std::string proto = c.storageSsl ? "https" : "http";
    return proto + "://" + host + canonicalUri + "?" + cqs + "&X-Amz-Signature=" + sig;
}

// ── Alibaba Cloud OSS presigned URL (signature v1, HMAC-SHA1). ────────────
inline std::string presignOss(const std::string &method, const std::string &key, int ttl) {
    const auto &c = AppConfig::instance();
    long long expires = (long long)std::time(nullptr) + ttl;
    std::string resource = "/" + c.storageBucket + "/" + key;
    // StringToSign = VERB\nContent-MD5\nContent-Type\nExpires\nCanonicalizedResource
    std::string sts = method + "\n\n\n" + std::to_string(expires) + "\n" + resource;
    std::string sig = base64(hmacSha1Raw(c.storageSecretAccessKey, sts));

    std::string proto = c.storageSsl ? "https" : "http";
    std::string host = c.storageBucket + "." + stripScheme(c.storageEndpoint);
    return proto + "://" + host + "/" + uriEncode(key, false) +
           "?OSSAccessKeyId=" + uriEncode(c.storageAccessKeyId) +
           "&Expires=" + std::to_string(expires) +
           "&Signature=" + uriEncode(sig);
}

// ── libcurl request keluar (PUT/DELETE) untuk driver remote via URL presigned.
//    Blocking di worker thread, resume ke event loop asal (pola HttpFetch.h). ─
struct CurlResult { long status{0}; std::string error; };

inline size_t curlReadCb(char *buffer, size_t size, size_t nitems, void *userdata) {
    auto *st = static_cast<std::pair<const std::string *, size_t> *>(userdata);
    size_t remaining = st->first->size() - st->second;
    size_t n = std::min(size * nitems, remaining);
    if (n) { std::memcpy(buffer, st->first->data() + st->second, n); st->second += n; }
    return n;
}
inline size_t curlDiscardCb(char *, size_t s, size_t n, void *) { return s * n; }

inline CurlResult sendBlocking(const std::string &method, const std::string &url,
                               const std::string &body, const std::vector<std::string> &headers,
                               long timeoutSec) {
    static std::once_flag f;
    std::call_once(f, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
    CurlResult r;
    CURL *c = curl_easy_init();
    if (!c) { r.error = "curl_easy_init gagal"; return r; }
    curl_slist *hdrs = nullptr;
    for (const auto &h : headers) hdrs = curl_slist_append(hdrs, h.c_str());
    std::pair<const std::string *, size_t> rd{&body, 0};

    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeoutSec);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, curlDiscardCb);
    if (method == "PUT") {
        curl_easy_setopt(c, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(c, CURLOPT_READFUNCTION, curlReadCb);
        curl_easy_setopt(c, CURLOPT_READDATA, &rd);
        curl_easy_setopt(c, CURLOPT_INFILESIZE_LARGE, (curl_off_t)body.size());
    } else if (method == "DELETE") {
        curl_easy_setopt(c, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    if (hdrs) curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);

    CURLcode rc = curl_easy_perform(c);
    if (rc != CURLE_OK) r.error = curl_easy_strerror(rc);
    else curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &r.status);
    if (hdrs) curl_slist_free_all(hdrs);
    curl_easy_cleanup(c);
    return r;
}

struct SendAwaiter : public drogon::CallbackAwaiter<CurlResult> {
    SendAwaiter(std::string m, std::string u, std::string b, std::vector<std::string> h, long t)
        : m_(std::move(m)), u_(std::move(u)), b_(std::move(b)), h_(std::move(h)), t_(t) {}
    void await_suspend(std::coroutine_handle<> handle) {
        auto *loop = trantor::EventLoop::getEventLoopOfCurrentThread();
        std::thread([this, handle, loop]() {
            setValue(sendBlocking(m_, u_, b_, h_, t_));
            if (loop) loop->queueInLoop([handle]() { handle.resume(); });
            else handle.resume();
        }).detach();
    }
    std::string m_, u_, b_;
    std::vector<std::string> h_;
    long t_;
};

}  // namespace detail

// URL render untuk sebuah key objek (dipanggil saat request, per NodeAdmin
// fileService.getSignedUrl). local → relatif /storage/<key>; oss/s3 → presigned
// absolut. Fallback anggun bila kredensial remote kosong (kembalikan /<key>).
inline std::string url(const std::string &key, int ttlSeconds = 6 * 3600) {
    if (key.empty()) return "";
    const auto &c = AppConfig::instance();
    if (c.storageDriver == "local") {
        std::string u = std::string(kLocalUrlPrefix) + "/" + key;
        // rapikan duplikasi '/'
        std::string out;
        for (char ch : u) { if (ch == '/' && !out.empty() && out.back() == '/') continue; out += ch; }
        return out;
    }
    if (c.storageAccessKeyId.empty() || c.storageSecretAccessKey.empty())
        return (key[0] == '/') ? key : "/" + key;  // belum dikonfigurasi
    if (c.storageDriver == "s3") return detail::presignS3("GET", key, ttlSeconds);
    return detail::presignOss("GET", key, ttlSeconds);
}

// Tulis berkas lokal (mkdir -p). Dipakai fast-path driver local.
inline void saveLocal(const std::string &key, const char *data, size_t len) {
    auto path = baseDir() / key;
    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) throw std::runtime_error("storage: gagal menulis " + path.string());
    ofs.write(data, (std::streamsize)len);
}

inline void removeLocal(const std::string &key) {
    std::error_code ec;
    std::filesystem::remove(baseDir() / key, ec);
}

// Simpan objek di bawah `key`. local → disk; oss/s3 → presigned PUT.
inline drogon::Task<void> save(std::string key, std::string data, std::string contentType) {
    const auto &c = AppConfig::instance();
    if (c.storageDriver == "local") {
        saveLocal(key, data.data(), data.size());
        co_return;
    }
    std::string putUrl = (c.storageDriver == "s3") ? detail::presignS3("PUT", key, 900)
                                                   : detail::presignOss("PUT", key, 900);
    // S3: SignedHeaders=host + UNSIGNED-PAYLOAD → Content-Type tak ditandatangani (boleh dikirim).
    // OSS: ditandatangani dgn Content-Type kosong → JANGAN kirim Content-Type.
    std::vector<std::string> headers;
    if (c.storageDriver == "s3" && !contentType.empty())
        headers.push_back("Content-Type: " + contentType);
    else
        headers.push_back("Content-Type:");  // tekan default curl agar cocok tanda tangan OSS
    auto r = co_await detail::SendAwaiter("PUT", putUrl, std::move(data), headers, 30);
    if (!r.error.empty() || r.status < 200 || r.status >= 300) {
        throw std::runtime_error("storage: upload gagal (" +
                                 (r.error.empty() ? "HTTP " + std::to_string(r.status) : r.error) + ")");
    }
    co_return;
}

// Hapus objek. local → unlink; oss/s3 → presigned DELETE (best-effort).
inline drogon::Task<void> remove(std::string key) {
    const auto &c = AppConfig::instance();
    if (c.storageDriver == "local") {
        removeLocal(key);
        co_return;
    }
    std::string delUrl = (c.storageDriver == "s3") ? detail::presignS3("DELETE", key, 900)
                                                   : detail::presignOss("DELETE", key, 900);
    co_await detail::SendAwaiter("DELETE", delUrl, "", {}, 30);
    co_return;  // idempotent: abaikan error hapus
}

}  // namespace storage
