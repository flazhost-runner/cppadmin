#include "AuthFilter.h"
#include "include/AppConfig.h"
#include "include/helpers/JwtHelper.h"
#include "include/helpers/JwtCookieHelper.h"
#include "include/helpers/JwtBlacklist.h"
#include <drogon/utils/coroutine.h>
#include <drogon/HttpResponse.h>
#include <json/json.h>

void AuthFilter::doFilter(const drogon::HttpRequestPtr &req,
                           drogon::FilterCallback      &&fcb,
                           drogon::FilterChainCallback &&fccb) {
    const std::string &path = req->path();
    bool isApi = (path.rfind("/api/", 0) == 0);

    // ── API path: expect "Authorization: Bearer <token>" ──────────────────────
    if (isApi) {
        auto auth = req->getHeader("Authorization");
        std::string token;
        if (auth.rfind("Bearer ", 0) == 0) token = auth.substr(7);

        if (!token.empty()) {
            auto result = jwt_helper::verify(token, AppConfig::instance().jwtSecret);
            if (result.valid) {
                req->attributes()->insert("currentUser",  result.sub);
                req->attributes()->insert("currentToken", token);
                fccb();
                return;
            }
        }
        auto body = std::make_shared<Json::Value>();
        (*body)["success"] = false;
        (*body)["message"] = "Unauthorized";
        auto r = drogon::HttpResponse::newHttpJsonResponse(*body);
        r->setStatusCode(drogon::k401Unauthorized);
        fcb(r);
        return;
    }

    // ── Web path: JWT in httpOnly cookie ──────────────────────────────────────
    std::string token = JwtCookie::get(req);
    if (!token.empty()) {
        auto jwtResult = jwt_helper::verify(token, AppConfig::instance().jwtSecret);
        if (jwtResult.valid && !jwtResult.jti.empty()) {
            // Blacklist check is async — capture callbacks before async_run
            auto fcbPtr  = std::make_shared<drogon::FilterCallback>(std::move(fcb));
            auto fccbPtr = std::make_shared<drogon::FilterChainCallback>(std::move(fccb));
            drogon::async_run([jwtResult, req, fcbPtr, fccbPtr]() -> drogon::Task<void> {
                bool blacklisted = false;
                try {
                    blacklisted = co_await JwtBlacklist::isBlacklisted(jwtResult.jti);
                } catch (...) {
                    // DB error — fail open to avoid locking users out on transient errors
                }
                if (blacklisted) {
                    (*fcbPtr)(drogon::HttpResponse::newRedirectionResponse("/auth/login"));
                    co_return;
                }
                req->attributes()->insert("currentUser",   jwtResult.sub);
                req->attributes()->insert("userRolesJson", jwtResult.roles);
                (*fccbPtr)();
            });
            return;
        }
    }

    // Not authenticated → redirect to login
    fcb(drogon::HttpResponse::newRedirectionResponse("/auth/login"));
}
