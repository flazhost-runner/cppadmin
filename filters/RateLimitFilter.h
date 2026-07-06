#pragma once
#include <drogon/HttpFilter.h>
#include <utility>

namespace ratelimit {
// Shared per-IP sliding-window limiter (Redis INCR + EXPIRE).
// Fails open when Redis is unavailable (dev mode without Redis).
void doRateLimit(const drogon::HttpRequestPtr &req,
                 drogon::FilterCallback      &&fcb,
                 drogon::FilterChainCallback &&fccb,
                 int maxRequests,
                 int windowSecs);
}  // namespace ratelimit

// CRTP base so every concrete limiter registers under its OWN class name in
// Drogon's DrClassMap. (drogon::HttpFilter<T> registers T only — subclassing
// a concrete filter does NOT register the subclass, which made routes fail
// with "filter AuthLimitFilter/OtpLimitFilter not found".)
template <typename T>
class RateLimitFilterBase : public drogon::HttpFilter<T> {
public:
    RateLimitFilterBase(int maxRequests, int windowSecs)
        : maxRequests_(maxRequests), windowSecs_(windowSecs) {}

    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override {
        ratelimit::doRateLimit(req, std::move(fcb), std::move(fccb),
                               maxRequests_, windowSecs_);
    }

private:
    int maxRequests_;
    int windowSecs_;
};

// Generic limiter: 10 requests per 60 seconds per IP.
class RateLimitFilter : public RateLimitFilterBase<RateLimitFilter> {
public:
    RateLimitFilter() : RateLimitFilterBase(10, 60) {}
};
