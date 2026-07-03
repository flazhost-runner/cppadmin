#include "AuthWebController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
#include "../../../include/helpers/JwtHelper.h"
#include "../../../include/helpers/JwtCookieHelper.h"
#include "../../../include/helpers/JwtBlacklist.h"
#include "../../../include/helpers/UuidGen.h"
#include "../../../include/AppConfig.h"
#include <json/json.h>
#include <chrono>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpResponse;

// Load setting and inject logo/login_image into view data (best-effort — falls back to defaults).
static drogon::Task<void> injectSettingAssets(drogon::HttpViewData &data,
                                               std::shared_ptr<ISettingService> svc) {
    try {
        auto s = co_await svc->findFirst();
        data["settingLogo"]       = std::string(s.getLogo()       ? *s.getLogo()       : "");
        data["settingLoginImage"] = std::string(s.getLoginImage() ? *s.getLoginImage() : "");
        data["settingTheme"]      = s.getValueOfTheme();
    } catch (...) {
        data["settingLogo"]       = std::string("/be/default/vendor/fontawesome-free/svgs/solid/chart-line.svg");
        data["settingLoginImage"] = std::string("");
        data["settingTheme"]      = std::string("blue");
    }
}

// Build and sign a web JWT containing userId, email, jti, and roles.
// Returns the signed token string. Caller is responsible for setting the cookie.
static std::string issueWebJwt(const std::string &userId,
                                const std::string &email,
                                const std::string &rolesJson) {
    auto &cfg = AppConfig::instance();
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    Json::Value payload;
    payload["sub"]   = userId;
    payload["email"] = email;
    payload["jti"]   = newUuid();
    payload["roles"] = rolesJson;
    payload["iat"]   = (Json::Int64)now;
    payload["exp"]   = (Json::Int64)(now + cfg.jwtExpireSeconds());

    return jwt_helper::sign(payload, cfg.jwtSecret);
}

// ── showLogin ─────────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::showLogin(HttpRequestPtr req) {
    drogon::HttpViewData data;
    co_await injectSettingAssets(data, settingSvc_);
    prepareViewData(data, req, data.get<std::string>("settingTheme"));
    co_return renderView("views::be::admin::auth::login", data);
}

// ── postLogin ─────────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::postLogin(HttpRequestPtr req) {
    std::string email    = req->getParameter("email");
    std::string password = req->getParameter("password");
    std::string loginErr;
    try {
        auto result = co_await auth_->login(email, password);
        std::string userId = result.user.getValueOfId();

        // Fetch roles to embed in web JWT
        Json::Value rolesArr(Json::arrayValue);
        try {
            auto roles = co_await userSvc_->rolesOf(userId);
            for (const auto &r : roles) rolesArr.append(r.getValueOfName());
        } catch (...) {}
        Json::FastWriter w;
        std::string rolesJson = w.write(rolesArr);
        if (!rolesJson.empty() && rolesJson.back() == '\n') rolesJson.pop_back();

        std::string token = issueWebJwt(userId, result.user.getValueOfEmail(), rolesJson);
        auto resp = HttpResponse::newRedirectionResponse("/admin/v1/dashboard");
        JwtCookie::set(resp, token, AppConfig::instance().jwtExpireSeconds());
        co_return resp;
    } catch (const std::exception &e) {
        loginErr = e.what();
    }
    // co_await not permitted inside catch — render error response outside the catch block
    drogon::HttpViewData data;
    co_await injectSettingAssets(data, settingSvc_);
    prepareViewData(data, req, data.get<std::string>("settingTheme"));
    Json::Value arr(Json::arrayValue);
    arr.append(loginErr);
    Json::FastWriter w;
    data["errorMessages"] = w.write(arr);
    co_return renderView("views::be::admin::auth::login", data);
}

// ── showRegister ──────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::showRegister(HttpRequestPtr req) {
    drogon::HttpViewData data;
    co_await injectSettingAssets(data, settingSvc_);
    prepareViewData(data, req, data.get<std::string>("settingTheme"));
    co_return renderView("views::be::admin::auth::signup", data);
}

// ── postRegister ──────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::postRegister(HttpRequestPtr req) {
    RegisterInput input;
    input.name     = req->getParameter("name");
    input.email    = req->getParameter("email");
    input.password = req->getParameter("password");
    input.phone    = req->getParameter("phone");

    auto user  = co_await auth_->registerUser(std::move(input));
    // New users have no roles yet; JWT carries an empty array
    std::string token = issueWebJwt(user.getValueOfId(), user.getValueOfEmail(), "[]");
    auto resp = HttpResponse::newRedirectionResponse("/admin/v1/dashboard");
    JwtCookie::set(resp, token, AppConfig::instance().jwtExpireSeconds());
    co_return resp;
}

// ── postLogout ────────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::postLogout(HttpRequestPtr req) {
    // Blacklist the current JWT so it cannot be reused before natural expiry
    std::string token = JwtCookie::get(req);
    if (!token.empty()) {
        auto jwtResult = jwt_helper::verify(token, AppConfig::instance().jwtSecret);
        if (jwtResult.valid && !jwtResult.jti.empty()) {
            try {
                co_await JwtBlacklist::blacklist(jwtResult.jti, jwtResult.exp);
            } catch (...) {}
        }
    }
    // Clear session (flash messages) and cookie
    req->session()->clear();
    auto resp = HttpResponse::newRedirectionResponse("/auth/login");
    JwtCookie::clear(resp);
    co_return resp;
}

// ── showResetReq ──────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::showResetReq(HttpRequestPtr req) {
    drogon::HttpViewData data;
    co_await injectSettingAssets(data, settingSvc_);
    prepareViewData(data, req, data.get<std::string>("settingTheme"));
    co_return renderView("views::be::admin::auth::reset_req", data);
}

// ── postResetReq ──────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::postResetReq(HttpRequestPtr req) {
    ResetRequestInput input;
    input.email = req->getParameter("email");
    co_await auth_->requestPasswordReset(std::move(input));
    Flash::setSuccess(req, "OTP Send Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/auth/reset/proc");
}

// ── showResetProc ─────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::showResetProc(HttpRequestPtr req) {
    drogon::HttpViewData data;
    co_await injectSettingAssets(data, settingSvc_);
    prepareViewData(data, req, data.get<std::string>("settingTheme"));
    co_return renderView("views::be::admin::auth::reset_proc", data);
}

// ── postResetProc ─────────────────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthWebController::postResetProc(HttpRequestPtr req) {
    ResetProcessInput input;
    input.email       = req->getParameter("email");
    input.otp         = req->getParameter("otp");
    input.newPassword = req->getParameter("new_password");
    co_await auth_->processPasswordReset(std::move(input));
    Flash::setSuccess(req, "Reset Password Success.");
    co_return HttpResponse::newRedirectionResponse("/auth/login");
}
