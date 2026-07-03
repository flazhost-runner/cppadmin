#include "DashboardController.h"
#include "../../../services/DashboardService.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <drogon/utils/HttpConstraint.h>
#include <memory>

using CS = std::vector<drogon::internal::HttpConstraint>;

void registerDashboardRoutes() {
    auto ctrl = std::make_shared<DashboardController>(
        std::make_shared<DashboardService>());

    CS withAll{drogon::Get, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};

    ROUTE_REG("dashboard.index", "GET", "/admin/v1/dashboard");

    drogon::app().registerHandler("/admin/v1/dashboard",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->index(req));
        },
        withAll);

    drogon::app().registerHandler("/admin",
        [](const drogon::HttpRequestPtr &,
           std::function<void(const drogon::HttpResponsePtr &)> &&cb) {
            cb(drogon::HttpResponse::newRedirectionResponse("/admin/v1/dashboard"));
        },
        {drogon::Get});
}
