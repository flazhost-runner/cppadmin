#pragma once
#include "RateLimitFilter.h"

// authLimiter: 10 requests per 15 minutes per IP
// Apply to POST /auth/login, POST /auth/signup, POST /auth/reset/request
class AuthLimitFilter : public RateLimitFilter {
public:
    AuthLimitFilter() : RateLimitFilter(10, 900) {}
};
