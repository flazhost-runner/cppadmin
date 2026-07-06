#include "SettingController.h"
#include "../../../services/SettingService.h"
#include "../../../services/FeTemplateService.h"
#include "../../../services/FeCatalogService.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <drogon/utils/HttpConstraint.h>
#include <memory>

using CS = std::vector<drogon::internal::HttpConstraint>;

void registerSettingRoutes() {
    auto feTemplate = std::make_shared<FeTemplateService>();
    auto catalog    = std::make_shared<FeCatalogService>();
    auto svc  = std::make_shared<SettingService>(feTemplate);
    auto ctrl = std::make_shared<SettingController>(svc, catalog);

    CS withAll{drogon::Get, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};
    CS withAllPut{drogon::Put, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};
    CS withAllPost{drogon::Post, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};

    ROUTE_REG("setting.edit",       "GET", "/admin/v1/setting");
    ROUTE_REG("setting.update",     "PUT", "/admin/v1/setting");
    ROUTE_REG("setting.fe_preview", "GET", "/admin/v1/setting/fe-preview/{slug}");

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

    // Preview HTML 1 template FE (untuk thumbnail iframe + modal di switcher)
    drogon::app().registerHandler("/admin/v1/setting/fe-preview/{slug}",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb,
               std::string slug)
               -> drogon::Task<void> {
            cb(co_await ctrl->fePreview(req, std::move(slug)));
        },
        withAll);
}
