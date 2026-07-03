#pragma once
#include <drogon/HttpController.h>
#include "../../../services/IAuthService.h"
#include "../../../services/AuthService.h"
#include "../../../services/ISettingService.h"
#include "../../../services/SettingService.h"
#include "../../../services/IUserService.h"
#include "../../../services/UserService.h"
#include <memory>

class AuthWebController : public drogon::HttpController<AuthWebController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AuthWebController::showLogin,    "/auth/login",    drogon::Get,  "CsrfFilter");
    ADD_METHOD_TO(AuthWebController::postLogin,    "/auth/login",    drogon::Post, "AuthLimitFilter","CsrfFilter");
    ADD_METHOD_TO(AuthWebController::showRegister, "/auth/signup", drogon::Get,  "CsrfFilter");
    ADD_METHOD_TO(AuthWebController::postRegister, "/auth/signup", drogon::Post, "AuthLimitFilter","CsrfFilter");
    ADD_METHOD_TO(AuthWebController::postLogout,   "/auth/logout",   drogon::Post, "AuthFilter","CsrfFilter");
    ADD_METHOD_TO(AuthWebController::showResetReq, "/admin/v1/auth/reset/req",     drogon::Get,  "CsrfFilter");
    ADD_METHOD_TO(AuthWebController::postResetReq, "/admin/v1/auth/reset/request", drogon::Post, "AuthLimitFilter","CsrfFilter");
    ADD_METHOD_TO(AuthWebController::showResetProc,"/admin/v1/auth/reset/proc",    drogon::Get,  "CsrfFilter");
    ADD_METHOD_TO(AuthWebController::postResetProc,"/admin/v1/auth/reset/process", drogon::Post, "OtpLimitFilter","CsrfFilter");
    METHOD_LIST_END

    // Default ctor for Drogon auto-instantiation (Drogon 1.8.7 — no registerObject)
    AuthWebController()
        : auth_(std::make_shared<AuthService>())
        , settingSvc_(std::make_shared<SettingService>())
        , userSvc_(std::make_shared<UserService>()) {}

    // Explicit ctor for test injection
    explicit AuthWebController(std::shared_ptr<IAuthService> auth,
                               std::shared_ptr<ISettingService> setting = nullptr,
                               std::shared_ptr<IUserService> userSvc = nullptr)
        : auth_(std::move(auth))
        , settingSvc_(setting ? std::move(setting) : std::make_shared<SettingService>())
        , userSvc_(userSvc ? std::move(userSvc) : std::make_shared<UserService>()) {}

    drogon::Task<drogon::HttpResponsePtr> showLogin(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> postLogin(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> showRegister(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> postRegister(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> postLogout(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> showResetReq(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> postResetReq(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> showResetProc(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> postResetProc(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IAuthService>     auth_;
    std::shared_ptr<ISettingService>  settingSvc_;
    std::shared_ptr<IUserService>     userSvc_;
};
