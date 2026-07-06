#pragma once
#include "RateLimitFilter.h"

// otpLimiter: 5 requests per 15 minutes per IP
// Apply to POST /auth/reset/process
class OtpLimitFilter : public RateLimitFilterBase<OtpLimitFilter> {
public:
    OtpLimitFilter() : RateLimitFilterBase(5, 900) {}
};
