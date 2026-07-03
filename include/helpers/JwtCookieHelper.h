#pragma once
#include <drogon/HttpRequest.h>
#include <drogon/HttpResponse.h>
#include "../AppConfig.h"
#include <string>

// Manages the httpOnly JWT auth cookie (web stateless auth).
// Cookie name: auth_token; HttpOnly; SameSite=Lax; Secure (production only).
// SameSite=Lax blocks cross-origin POST — doubles as CSRF protection for
// authenticated routes, so no separate token is needed for those.
namespace JwtCookie {

constexpr const char *COOKIE_NAME = "auth_token";

inline std::string get(const drogon::HttpRequestPtr &req) {
    return req->getCookie(COOKIE_NAME);
}

inline void set(const drogon::HttpResponsePtr &resp,
                const std::string &token,
                long long maxAgeSeconds) {
    std::string cookie = std::string(COOKIE_NAME) + "=" + token
        + "; HttpOnly; Path=/; SameSite=Lax; Max-Age="
        + std::to_string(maxAgeSeconds);
    if (AppConfig::instance().isProduction()) cookie += "; Secure";
    resp->addHeader("Set-Cookie", cookie);
}

inline void clear(const drogon::HttpResponsePtr &resp) {
    resp->addHeader("Set-Cookie",
        std::string(COOKIE_NAME) + "=; HttpOnly; Path=/; SameSite=Lax; Max-Age=0");
}

} // namespace JwtCookie
