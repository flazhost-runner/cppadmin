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

    // Satu halaman form penuh selaras NodeAdmin: GET menampilkan form, PUT menyimpan.
    ROUTE_REG("profile.show",   "GET", "/admin/v1/profile");
    ROUTE_REG("profile.update", "PUT", "/admin/v1/profile/update");

    drogon::app().registerHandler("/admin/v1/profile",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->show(req)); },
        withGet);

    drogon::app().registerHandler("/admin/v1/profile/update",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->update(req)); },
        withPut);

    // POST companion untuk _method=PUT. Drogon mencocokkan rute SEBELUM filter jalan,
    // jadi MethodOverrideFilter tidak pernah sempat mengubah POST→PUT: tanpa handler
    // POST di path ini, "Simpan" pada form profil hanya berakhir 404. Pola yang sama
    // sudah dipakai di SettingRoutes dan AccessRoutes.
    drogon::app().registerHandler("/admin/v1/profile/update",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> { cb(co_await ctrl->update(req)); },
        withPost);
}
