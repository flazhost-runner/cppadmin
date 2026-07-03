#include "ComponentsController.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <drogon/utils/HttpConstraint.h>
#include <memory>

using CS = std::vector<drogon::internal::HttpConstraint>;

void registerComponentsRoutes() {
    auto ctrl = std::make_shared<ComponentsController>();

    CS withAll{drogon::Get, "MethodOverrideFilter", "CsrfFilter", "AuthFilter", "RbacFilter"};

    ROUTE_REG("components.index", "GET", "/admin/v1/components");

    drogon::app().registerHandler("/admin/v1/components",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->index(req));
        },
        withAll);
}
