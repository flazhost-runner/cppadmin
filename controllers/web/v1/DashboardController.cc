#include "DashboardController.h"
#include "../../../include/helpers/ViewHelper.h"

drogon::Task<drogon::HttpResponsePtr>
DashboardController::index(drogon::HttpRequestPtr req) {
    auto attrs = req->getAttributes();
    std::string uid = attrs->find("currentUser")
                    ? attrs->get<std::string>("currentUser") : "";

    auto stats = co_await svc_->getStats(uid);

    drogon::HttpViewData data;
    prepareViewData(data, req);

    data["totalUsers"]        = std::to_string(stats.totalUsers);
    data["totalRoles"]        = std::to_string(stats.totalRoles);
    data["totalPerms"]        = std::to_string(stats.totalPerms);
    data["currentUserName"]   = stats.currentUserName;
    data["currentUserEmail"]  = stats.currentUserEmail;

    co_return renderView("views::be::admin::dashboard::index", data);
}
