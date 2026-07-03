#pragma once
#include <drogon/HttpFilter.h>

// Per-IP sliding-window rate limiter using Redis INCR + EXPIRE.
// Register on sensitive endpoints: login, register, reset-OTP.
// maxRequests_ / windowSecs_ set by constructor or subclass.
class RateLimitFilter : public drogon::HttpFilter<RateLimitFilter> {
public:
    explicit RateLimitFilter(int maxRequests = 10, int windowSecs = 60)
        : maxRequests_(maxRequests), windowSecs_(windowSecs) {}

    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override;

private:
    int maxRequests_{10};
    int windowSecs_{60};
};
