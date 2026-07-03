#include "SecurityHeadersFilter.h"
#include <drogon/HttpResponse.h>

void SecurityHeadersFilter::doFilter(const drogon::HttpRequestPtr &req,
                                      drogon::FilterCallback      &&fcb,
                                      drogon::FilterChainCallback &&fccb) {
    fccb();  // run controller first, then set headers on response via advice
    // Note: Drogon advices are used to set headers post-handler.
    // For pre-handler security we can pass through and rely on a post-handler advice.
    // Actually inject headers here via a sync advice registered in main.cc.
    (void)req; (void)fcb;
}
