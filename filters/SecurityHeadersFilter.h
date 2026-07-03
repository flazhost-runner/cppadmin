#pragma once
#include <drogon/HttpFilter.h>

// Global filter — adds HSTS, X-Frame-Options, X-Content-Type-Options, CSP, etc.
// Register globally in main.cc via app().registerFilter().
class SecurityHeadersFilter : public drogon::HttpFilter<SecurityHeadersFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override;
};
