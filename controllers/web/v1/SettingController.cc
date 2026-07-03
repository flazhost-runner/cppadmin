#include "SettingController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
#include "../../../include/AppError.h"

drogon::Task<drogon::HttpResponsePtr>
SettingController::edit(drogon::HttpRequestPtr req) {
    auto setting = co_await svc_->findFirst();

    drogon::HttpViewData data;
    prepareViewData(data, req, setting.getValueOfTheme());

    auto sv = [](const std::string *p) -> std::string { return p ? *p : ""; };
    data["sName"]        = sv(setting.getName());
    data["sDescription"] = sv(setting.getDescription());
    data["sEmail"]       = sv(setting.getEmail());
    data["sPhone"]       = sv(setting.getPhone());
    data["sAddress"]     = sv(setting.getAddress());
    data["sCopyright"]   = sv(setting.getCopyright());
    data["sTheme"]       = setting.getValueOfTheme();
    data["sInitial"]     = sv(setting.getInitial());
    data["sIcon"]        = sv(setting.getIcon());
    data["sLogo"]        = sv(setting.getLogo());
    data["sFavicon"]     = sv(setting.getFavicon());
    data["sLoginImage"]  = sv(setting.getLoginImage());
    data["sFeTemplate"]  = sv(setting.getFeTemplate());

    co_return renderView("views::be::admin::setting::edit", data);
}

drogon::Task<drogon::HttpResponsePtr>
SettingController::update(drogon::HttpRequestPtr req) {
    auto setting = co_await svc_->findFirst();
    std::string id = setting.getValueOfId();

    SettingUpdateInput input;
    input.name        = req->getParameter("name");
    input.description = req->getParameter("description");
    input.icon        = req->getParameter("icon");
    input.logo        = req->getParameter("logo");
    input.favicon     = req->getParameter("favicon");
    input.loginImage  = req->getParameter("login_image");
    input.phone       = req->getParameter("phone");
    input.address     = req->getParameter("address");
    input.email       = req->getParameter("email");
    input.copyright   = req->getParameter("copyright");
    input.theme       = req->getParameter("theme");
    input.feTemplate  = req->getParameter("fe_template");

    auto sAttrs = req->getAttributes();
    std::string actor = sAttrs->find("currentUser") ? sAttrs->get<std::string>("currentUser") : "system";

    co_await svc_->update(id, input, actor);
    Flash::setSuccess(req, "Save Setting Success.");
    co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/setting");
}
