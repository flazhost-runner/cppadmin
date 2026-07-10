#include "CsrfFilter.h"
#include "../include/helpers/JwtHelper.h"
#include "../include/helpers/JwtCookieHelper.h"
#include "../include/AppConfig.h"
#include <drogon/HttpResponse.h>
#include <openssl/crypto.h>
#include <chrono>

// HMAC-SHA256(JWT_SECRET, "csrf:" + jti), base64url-encoded.
static std::string hmacCsrfToken(const std::string &jti) {
    const std::string &secret = AppConfig::instance().jwtSecret;
    std::string raw = jwt_helper::hmacSha256(secret, "csrf:" + jti);
    return jwt_helper::base64UrlEncode(
        reinterpret_cast<const unsigned char *>(raw.data()), raw.size());
}

// HMAC-SHA256(JWT_SECRET, "login-csrf:" + ip + ":" + ua + ":" + bucket), base64url-encoded.
static std::string loginCsrfToken(const std::string &ip,
                                   const std::string &ua,
                                   long long bucket) {
    const std::string &secret = AppConfig::instance().jwtSecret;
    std::string data = "login-csrf:" + ip + ":" + ua + ":" + std::to_string(bucket);
    std::string raw = jwt_helper::hmacSha256(secret, data);
    return jwt_helper::base64UrlEncode(
        reinterpret_cast<const unsigned char *>(raw.data()), raw.size());
}

static bool timingSafeEq(const std::string &a, const std::string &b) {
    if (a.empty() || b.empty() || a.size() != b.size()) return false;
    return CRYPTO_memcmp(a.data(), b.data(), a.size()) == 0;
}

// Real client IP for the unauthenticated login-CSRF binding.
// Behind a reverse proxy (CapRover / Docker-Swarm ingress) getPeerAddr() is the
// internal proxy IP and can differ between the GET (renders form + token) and
// the POST (validates) → token mismatch → false 403. Prefer the first hop of
// X-Forwarded-For (the real client), falling back to getPeerAddr() when absent.
// The SAME derivation is used in generateToken and doFilter so GET/POST agree.
static std::string clientIp(const drogon::HttpRequestPtr &req) {
    std::string xff = req->getHeader("x-forwarded-for");
    if (!xff.empty()) {
        // "client, proxy1, proxy2" — take the first hop and trim whitespace.
        auto comma = xff.find(',');
        std::string first = (comma == std::string::npos) ? xff : xff.substr(0, comma);
        size_t b = first.find_first_not_of(" \t");
        size_t e = first.find_last_not_of(" \t");
        if (b != std::string::npos) return first.substr(b, e - b + 1);
    }
    return req->getPeerAddr().toIp();
}

std::string CsrfFilter::generateToken(const drogon::HttpRequestPtr &req) {
    auto attrs = req->getAttributes();
    if (attrs->find("csrfToken")) return attrs->get<std::string>("csrfToken");

    std::string tok;
    std::string jwtStr = JwtCookie::get(req);
    if (!jwtStr.empty()) {
        auto r = jwt_helper::verify(jwtStr, AppConfig::instance().jwtSecret);
        if (r.valid && !r.jti.empty()) {
            tok = hmacCsrfToken(r.jti);
        }
    }
    if (tok.empty()) {
        long long now = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        tok = loginCsrfToken(clientIp(req),
                              req->getHeader("user-agent"),
                              now / 3600);
    }
    attrs->insert("csrfToken", tok);
    return tok;
}

std::string CsrfFilter::extractToken(const drogon::HttpRequestPtr &req) {
    auto body = req->getParameter("_csrf");
    if (!body.empty()) return body;
    auto query = req->getQuery();
    std::string key = "_csrf=";
    auto pos = query.find(key);
    if (pos != std::string::npos) {
        auto val = query.substr(pos + key.size());
        auto end = val.find('&');
        if (end != std::string::npos) val = val.substr(0, end);
        if (!val.empty()) return val;
    }
    auto hdr = req->getHeader("x-csrf-token");
    if (!hdr.empty()) return hdr;
    return "";
}

void CsrfFilter::doFilter(const drogon::HttpRequestPtr &req,
                           drogon::FilterCallback      &&fcb,
                           drogon::FilterChainCallback &&fccb) {
    auto method = req->method();
    if (method == drogon::Get || method == drogon::Head || method == drogon::Options) {
        generateToken(req); // cache in "csrfToken" attribute for ViewHelper
        fccb();
        return;
    }
    const std::string &path = req->path();
    if (path.rfind("/api/", 0) == 0) {
        fccb();
        return;
    }

    std::string submitted = extractToken(req);

    // Authenticated path: verify HMAC(jti, secret)
    std::string jwtStr = JwtCookie::get(req);
    if (!jwtStr.empty()) {
        auto r = jwt_helper::verify(jwtStr, AppConfig::instance().jwtSecret);
        if (r.valid && !r.jti.empty()) {
            std::string expected = hmacCsrfToken(r.jti);
            if (!timingSafeEq(expected, submitted)) {
                auto resp = drogon::HttpResponse::newHttpResponse();
                resp->setStatusCode(drogon::k403Forbidden);
                resp->setContentTypeCode(drogon::CT_TEXT_HTML);
                resp->setBody("<h1>403 Invalid CSRF Token</h1>");
                fcb(resp);
                return;
            }
            fccb();
            return;
        }
    }

    // Unauthenticated path (login/register/reset): check current and previous hour bucket
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    long long bucket = now / 3600;
    std::string ip = clientIp(req);
    std::string ua = req->getHeader("user-agent");
    bool valid = timingSafeEq(loginCsrfToken(ip, ua, bucket),     submitted)
              || timingSafeEq(loginCsrfToken(ip, ua, bucket - 1), submitted);
    if (!valid) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k403Forbidden);
        resp->setContentTypeCode(drogon::CT_TEXT_HTML);
        resp->setBody("<h1>403 Invalid CSRF Token</h1>");
        fcb(resp);
        return;
    }
    fccb();
}
