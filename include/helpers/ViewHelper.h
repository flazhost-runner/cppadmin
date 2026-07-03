#pragma once
#include <drogon/drogon.h>
#include <drogon/HttpViewData.h>
#include "../AppConfig.h"
#include "HtmlEscape.h"
#include "FlashHelper.h"
#include <string>
#include <unordered_map>

// Theme CSS variables table — 5 standard themes matching NodeAdmin exactly
inline std::unordered_map<std::string, std::unordered_map<std::string, std::string>>
themeVars() {
    return {
        {"blue",   {{"primary","#3B82F6"},{"secondary","#60A5FA"},{"light","#EFF6FF"},{"dark","#1E40AF"}}},
        {"purple", {{"primary","#8B5CF6"},{"secondary","#A78BFA"},{"light","#F5F3FF"},{"dark","#5B21B6"}}},
        {"green",  {{"primary","#10B981"},{"secondary","#34D399"},{"light","#ECFDF5"},{"dark","#065F46"}}},
        {"orange", {{"primary","#F59E0B"},{"secondary","#FCD34D"},{"light","#FFFBEB"},{"dark","#92400E"}}},
        {"red",    {{"primary","#EF4444"},{"secondary","#F87171"},{"light","#FEF2F2"},{"dark","#991B1B"}}},
    };
}

// Inject theme CSS variables into view data (from settings.theme or default blue)
inline void injectTheme(drogon::HttpViewData &data,
                        const std::string &themeName = "blue") {
    auto all = themeVars();
    auto it = all.find(themeName);
    if (it == all.end()) it = all.find("blue");
    const auto &vars = it->second;
    data["themePrimary"]   = vars.at("primary");
    data["themeSecondary"] = vars.at("secondary");
    data["themeLight"]     = vars.at("light");
    data["themeDark"]      = vars.at("dark");
    data["themeName"]      = it->first;
}

// Inject CSRF token into view data.
// CsrfFilter::generateToken caches the computed token in request attribute "csrfToken"
// during the GET phase; this reads that cached value (empty string if filter didn't run).
inline void injectCsrf(drogon::HttpViewData &data,
                       const drogon::HttpRequestPtr &req) {
    auto attrs = req->getAttributes();
    std::string tok;
    if (attrs->find("csrfToken")) tok = attrs->get<std::string>("csrfToken");
    data["csrfToken"] = tok;
}

// Inject flash messages (consumed from session — one-time read)
inline void injectFlash(drogon::HttpViewData &data,
                        const drogon::HttpRequestPtr &req) {
    FlashData flash = Flash::consume(req);
    data["flashSuccess"]      = flash.success;
    data["flashError"]        = flash.error;
    data["flashFieldErrors"]  = flash.fieldErrorsJson;
    data["flashOldInput"]     = flash.oldInputJson;
}

// Inject app-level settings (appName, appUrl)
inline void injectAppSettings(drogon::HttpViewData &data) {
    auto &cfg = AppConfig::instance();
    data["appName"] = cfg.appName;
    data["appUrl"]  = cfg.appUrl;
}

// One-shot: inject everything needed by the admin layout
// AuthFilter stores userId as string in attributes["currentUser"].
// Controllers that need full user info (name/email) inject it separately.
inline void prepareViewData(drogon::HttpViewData &data,
                             const drogon::HttpRequestPtr &req,
                             const std::string &theme = "blue") {
    injectAppSettings(data);
    injectTheme(data, theme);
    injectCsrf(data, req);
    injectFlash(data, req);
    data["currentPath"] = req->path();

    auto attrs = req->getAttributes();
    if (attrs->find("currentUser")) {
        data["currentUserId"] = attrs->get<std::string>("currentUser");
    } else {
        data["currentUserId"]    = std::string{};
        data["currentUserName"]  = std::string{};
        data["currentUserEmail"] = std::string{};
        data["currentUserPic"]   = std::string{};
    }

    // userRolesJson is injected by AuthFilter from the JWT "roles" claim
    if (attrs->find("userRolesJson")) {
        data["userRolesJson"] = attrs->get<std::string>("userRolesJson");
    } else {
        data["userRolesJson"] = std::string{"[]"};
    }

    // errorMessages[] — default empty (controllers set this on validation failure)
    if (data.get<std::string>("errorMessages").empty()) {
        data["errorMessages"] = std::string{};
    }
}

// Helper: render view with text/html content-type
inline drogon::HttpResponsePtr renderView(const std::string &viewName,
                                           drogon::HttpViewData data) {
    auto resp = drogon::HttpResponse::newHttpViewResponse(viewName, data);
    resp->setContentTypeCodeAndCustomString(
        drogon::CT_TEXT_HTML, "text/html; charset=utf-8");
    return resp;
}
