#pragma once
#include <drogon/HttpFilter.h>

// Checks that the user is authenticated via JWT (web cookie or API Bearer).
// Web (path != /api/): reads "auth_token" httpOnly cookie, verifies JWT, checks
//   JTI blacklist (async DB). On success injects "currentUser" + "userRolesJson"
//   into request attributes. No valid JWT → redirect to /auth/login (302).
// API (/api/):          Bearer token → verify → inject "currentUser". Invalid → 401 JSON.
class AuthFilter : public drogon::HttpFilter<AuthFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override;
};
