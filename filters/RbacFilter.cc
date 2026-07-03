#include "RbacFilter.h"
#include "../include/RouteRegistry.h"
#include <drogon/HttpResponse.h>
#include <drogon/utils/coroutine.h>
#include <json/json.h>

// Async RBAC helpers (implemented in services/RbacHelper.cc)
drogon::Task<bool> userIsAdministratorAsync(const std::string &userId);
drogon::Task<bool> userHasAccessAsync(const std::string &userId,
                                       const std::string &routeName,
                                       const std::string &method);

void RbacFilter::doFilter(const drogon::HttpRequestPtr &req,
                           drogon::FilterCallback      &&fcb,
                           drogon::FilterChainCallback &&fccb) {
    auto attrs = req->getAttributes();
    if (!attrs->find("currentUser")) {
        auto resp = drogon::HttpResponse::newRedirectionResponse("/auth/login");
        fcb(resp);
        return;
    }
    std::string userId = attrs->get<std::string>("currentUser");
    if (userId.empty()) {
        auto resp = drogon::HttpResponse::newRedirectionResponse("/auth/login");
        fcb(resp);
        return;
    }
    std::string path   = req->path();
    bool isApi = (path.rfind("/api/", 0) == 0);
    std::string method = req->methodString();

    // Resolve route name from registry
    std::string routeName = RouteRegistry::instance().nameByMethodAndPath(method, path);

    // No named route registered → pass through (unregistered/public path)
    if (routeName.empty()) { fccb(); return; }

    // Capture callbacks (must be shared — they outlive this call)
    auto fcbPtr  = std::make_shared<drogon::FilterCallback>(std::move(fcb));
    auto fccbPtr = std::make_shared<drogon::FilterChainCallback>(std::move(fccb));

    drogon::async_run([userId, routeName, method, isApi, req, fcbPtr, fccbPtr]
                      () -> drogon::Task<void> {
        try {
            if (co_await userIsAdministratorAsync(userId)) {
                (*fccbPtr)();
                co_return;
            }
            if (co_await userHasAccessAsync(userId, routeName, method)) {
                (*fccbPtr)();
                co_return;
            }
        } catch (...) {
            // DB error → allow (fail-open prevents lockout on DB hiccup)
            (*fccbPtr)();
            co_return;
        }

        // Forbidden
        if (isApi) {
            Json::Value body;
            body["status"]  = false;
            body["message"] = "Forbidden";
            body["data"]    = Json::nullValue;
            auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
            resp->setStatusCode(drogon::k403Forbidden);
            (*fcbPtr)(resp);
        } else {
            req->session()->insert("flash_key",     std::string("error"));
            req->session()->insert("flash_message", std::string("Unauthorized."));
            std::string referer = req->getHeader("Referer");
            if (referer.empty()) referer = "/admin/v1/dashboard";
            auto resp = drogon::HttpResponse::newRedirectionResponse(referer);
            (*fcbPtr)(resp);
        }
    });
}
