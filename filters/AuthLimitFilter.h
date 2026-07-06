#pragma once
#include "RateLimitFilter.h"

// authLimiter: 10 requests per 15 minutes per IP
// Apply to POST /auth/login, POST /auth/signup, POST /auth/reset/request
class AuthLimitFilter : public RateLimitFilterBase<AuthLimitFilter> {
public:
    AuthLimitFilter() : RateLimitFilterBase(10, 900) {}
};
