#include "ProfileController.h"
#include "../../../services/UserService.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <drogon/utils/HttpConstraint.h>
#include <memory>

using CS = std::vector<drogon::internal::HttpConstraint>;

void registerProfileRoutes() {
    auto userSvc = std::make_shared<UserService>();
    auto ctrl    = std::make_shared<ProfileController>(userSvc);

    // Profile: no RbacFilter — every authenticated user can manage their own profile
    CS withGet{drogon::Get,  "MethodOverrideFilter", "CsrfFilter", "AuthFilter"};
    CS withPut{drogon::Put,  "MethodOverrideFilter", "CsrfFilter", "AuthFilter"};
    CS withPost{drogon::Post,"MethodOverrideFilter", "CsrfFilter", "AuthFilter"};

    ROUTE_REG("profile.show",            "GET",  "/admin/v1/profile");
    ROUTE_REG("profile.edit",            "GET",  "/admin/v1/profile/edit");
    ROUTE_REG("profile.update",          "PUT",  "/admin/v1/profile");
    ROUTE_REG("profile.edit_password",   "GET",  "/admin/v1/profile/password");
    ROUTE_REG("profile.update_password", "POST", "/admin/v1/profile/password");

    drogon::app().registerHandler("/admin/v1/profile",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->show(req)); },
        withGet);

    drogon::app().registerHandler("/admin/v1/profile/edit",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->edit(req)); },
        withGet);

    drogon::app().registerHandler("/admin/v1/profile",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->update(req)); },
        withPut);

    drogon::app().registerHandler("/admin/v1/profile/password",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->editPassword(req)); },
        withGet);

    drogon::app().registerHandler("/admin/v1/profile/password",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->updatePassword(req)); },
        withPost);
}
