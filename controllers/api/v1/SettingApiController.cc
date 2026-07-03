#include "SettingApiController.h"
#include <drogon/HttpResponse.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpResponse;

drogon::HttpResponsePtr SettingApiController::jsonOk(Json::Value body) {
    body["status"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(drogon::k200OK);
    return resp;
}

drogon::Task<HttpResponsePtr>
SettingApiController::show(HttpRequestPtr req) {
    auto setting = co_await svc_->findFirst();

    auto sv = [](const std::string *p) -> std::string { return p ? *p : ""; };

    Json::Value d;
    d["id"]          = setting.getValueOfId();
    d["name"]        = sv(setting.getName());
    d["description"] = sv(setting.getDescription());
    d["icon"]        = sv(setting.getIcon());
    d["logo"]        = sv(setting.getLogo());
    d["favicon"]     = sv(setting.getFavicon());
    d["loginImage"]  = sv(setting.getLoginImage());
    d["phone"]       = sv(setting.getPhone());
    d["address"]     = sv(setting.getAddress());
    d["email"]       = sv(setting.getEmail());
    d["copyright"]   = sv(setting.getCopyright());
    d["theme"]       = setting.getValueOfTheme();
    d["initial"]     = sv(setting.getInitial());
    d["feTemplate"]  = sv(setting.getFeTemplate());

    Json::Value body;
    body["message"] = "";
    body["data"]    = d;
    co_return jsonOk(std::move(body));
}
