#include "HomeController.h"
#include "../../../include/helpers/ViewHelper.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;

static std::string sv(const std::string *p) { return p ? *p : ""; }

drogon::Task<HttpResponsePtr>
HomeController::index(HttpRequestPtr req) {
    drogon::HttpViewData data;
    injectAppSettings(data);

    try {
        auto setting = co_await svc_->findFirst();
        data["settingName"]        = sv(setting.getName());
        data["settingLogo"]        = sv(setting.getLogo());
        data["settingInitial"]     = sv(setting.getInitial());
        data["settingDescription"] = sv(setting.getDescription());
        if (data.get<std::string>("appName").empty())
            data["appName"] = sv(setting.getName());
    } catch (...) {
        // Setting not found — render with defaults
        data["settingName"]    = std::string{"CppAdmin"};
        data["settingInitial"] = std::string{"Admin"};
    }

    co_return renderView("views::fe::deflt::index", data);
}
