#include "MethodOverrideFilter.h"
#include <drogon/HttpRequest.h>

void MethodOverrideFilter::doFilter(const drogon::HttpRequestPtr &req,
                                     drogon::FilterCallback      &&fcb,
                                     drogon::FilterChainCallback &&fccb) {
    if (req->method() == drogon::Post) {
        auto override_ = req->getParameter("_method");
        if (override_ == "PUT" || override_ == "put") {
            req->setMethod(drogon::Put);
        } else if (override_ == "DELETE" || override_ == "delete") {
            req->setMethod(drogon::Delete);
        } else if (override_ == "PATCH" || override_ == "patch") {
            req->setMethod(drogon::Patch);
        }
    }
    fccb();
}
