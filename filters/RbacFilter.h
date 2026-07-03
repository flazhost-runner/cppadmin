#pragma once
#include <drogon/HttpFilter.h>

// Route-driven RBAC — no hardcoded permission names.
// Uses RouteRegistry reverse-lookup: (method, path) → route name.
// Checks: does any of currentUser's roles have permission(name, method)?
// Administrator role bypasses all checks.
// Must run AFTER AuthFilter (needs "currentUser" attribute).
class RbacFilter : public drogon::HttpFilter<RbacFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override;
};
