#pragma once
#include <drogon/HttpFilter.h>
#include <string>

// Stateless CSRF protection — no session required, safe across multiple replicas.
//
// Token derivation:
//   Authenticated (valid auth_token JWT present):
//     token = base64url(HMAC-SHA256(JWT_SECRET, "csrf:" + jti))
//   Unauthenticated (login/register/reset — no JWT yet):
//     token = base64url(HMAC-SHA256(JWT_SECRET, "login-csrf:" + peerIp + ":" + userAgent + ":" + hourBucket))
//     hourBucket = unix_seconds / 3600 — tokens rotate hourly; filter accepts current and previous bucket.
//
// Token lookup on validation (body field _csrf → query param _csrf → header X-CSRF-Token).
// GET/HEAD/OPTIONS pass through; token is computed and cached in request attribute "csrfToken"
// for ViewHelper to inject into the rendered view.
class CsrfFilter : public drogon::HttpFilter<CsrfFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override;

    // Compute CSRF token for req (or return cached value from request attribute).
    static std::string generateToken(const drogon::HttpRequestPtr &req);

    // Read submitted token from request (body → query → header).
    static std::string extractToken(const drogon::HttpRequestPtr &req);
};
