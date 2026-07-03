#include "AuthWebController.h"
#include "../../../controllers/api/v1/AuthApiController.h"
#include "../../../include/RouteRegistry.h"
#include "../../../services/AuthService.h"
#include <drogon/drogon.h>

// Called from main.cc: registerAuthRoutes()
void registerAuthRoutes() {
    auto authSvc = std::make_shared<AuthService>();

    // Web routes
    ROUTE_REG("web.auth.login",            "GET",  "/auth/login");
    ROUTE_REG("web.auth.login.post",       "POST", "/auth/login");
    ROUTE_REG("web.auth.signup",      "GET",  "/auth/signup");
    ROUTE_REG("web.auth.signup.post", "POST", "/auth/signup");
    ROUTE_REG("web.auth.logout",           "POST", "/auth/logout");
    ROUTE_REG("admin.v1.auth.reset.req",     "GET",  "/admin/v1/auth/reset/req");
    ROUTE_REG("admin.v1.auth.reset.request", "POST", "/admin/v1/auth/reset/request");
    ROUTE_REG("admin.v1.auth.reset.proc",    "GET",  "/admin/v1/auth/reset/proc");
    ROUTE_REG("admin.v1.auth.reset.process", "POST", "/admin/v1/auth/reset/process");

    // API routes
    ROUTE_REG("api.v1.auth.login",           "POST", "/api/v1/auth/login");
    ROUTE_REG("api.v1.auth.register",        "POST", "/api/v1/auth/register");
    ROUTE_REG("api.v1.auth.reset.request",   "POST", "/api/v1/auth/reset/request");
    ROUTE_REG("api.v1.auth.reset.process",   "POST", "/api/v1/auth/reset/process");

    // HttpController subclasses with METHOD_LIST_BEGIN are auto-instantiated by Drogon.
    // We pre-create them here to force default construction with service wiring.
    // (registerObject not available in Drogon 1.8.7 — services created in default ctor)
    (void)std::make_shared<AuthWebController>(authSvc);
    (void)std::make_shared<AuthApiController>(authSvc);
}
