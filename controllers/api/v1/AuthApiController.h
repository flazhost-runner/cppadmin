#pragma once
#include <drogon/HttpController.h>
#include "../../../services/IAuthService.h"
#include "../../../services/AuthService.h"
#include <memory>

class AuthApiController : public drogon::HttpController<AuthApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthApiController::login,    "/api/v1/auth/login",         drogon::Post, "AuthLimitFilter");
    ADD_METHOD_TO(AuthApiController::register_,"/api/v1/auth/register",      drogon::Post, "AuthLimitFilter");
    ADD_METHOD_TO(AuthApiController::resetReq, "/api/v1/auth/reset/request", drogon::Post, "AuthLimitFilter");
    ADD_METHOD_TO(AuthApiController::resetProc,"/api/v1/auth/reset/process", drogon::Post, "OtpLimitFilter");
    METHOD_LIST_END

    // Default ctor for Drogon auto-instantiation (Drogon 1.8.7 — no registerObject)
    AuthApiController() : auth_(std::make_shared<AuthService>()) {}

    // Explicit ctor for test injection
    explicit AuthApiController(std::shared_ptr<IAuthService> auth)
        : auth_(std::move(auth)) {}

    drogon::Task<drogon::HttpResponsePtr> login(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> register_(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> resetReq(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> resetProc(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IAuthService> auth_;
};
