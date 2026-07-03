#pragma once
#include <drogon/HttpFilter.h>

// Reads ?_method=PUT|DELETE from POST requests and overrides the method BEFORE routing.
// Must be registered as a global pre-routing filter.
// Only POST→{PUT, PATCH, DELETE} is allowed; other override values are ignored.
class MethodOverrideFilter : public drogon::HttpFilter<MethodOverrideFilter> {
public:
    void doFilter(const drogon::HttpRequestPtr &req,
                  drogon::FilterCallback      &&fcb,
                  drogon::FilterChainCallback &&fccb) override;
};
