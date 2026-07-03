#include "SettingController.h"
#include "../../../services/SettingService.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <drogon/utils/HttpConstraint.h>
#include <memory>

using CS = std::vector<drogon::internal::HttpConstraint>;

void registerSettingRoutes() {
    auto svc  = std::make_shared<SettingService>();
    auto ctrl = std::make_shared<SettingController>(svc);

    CS withAll{drogon::Get, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};
    CS withAllPut{drogon::Put, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};
    CS withAllPost{drogon::Post, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};

    ROUTE_REG("setting.edit",   "GET", "/admin/v1/setting");
    ROUTE_REG("setting.update", "PUT", "/admin/v1/setting");

    drogon::app().registerHandler("/admin/v1/setting",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->edit(req));
        },
        withAll);

    drogon::app().registerHandler("/admin/v1/setting",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->update(req));
        },
        withAllPut);

    // POST handler for _method=PUT override (MethodOverrideFilter converts to PUT)
    drogon::app().registerHandler("/admin/v1/setting",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->update(req));
        },
        withAllPost);
}
