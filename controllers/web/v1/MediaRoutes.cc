#include "MediaController.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <memory>

using CS = std::vector<drogon::internal::HttpConstraint>;

void registerMediaRoutes() {
    auto ctrl = std::make_shared<MediaController>();

    CS mediaGet {drogon::Get,  "CsrfFilter", "AuthFilter", "RbacFilter"};
    CS mediaPost{drogon::Post, "CsrfFilter", "AuthFilter", "RbacFilter"};

    ROUTE_REG("admin.v1.media.list",   "GET",  "/admin/v1/media/list");
    ROUTE_REG("admin.v1.media.upload", "POST", "/admin/v1/media/upload");
    ROUTE_REG("admin.v1.media.delete", "POST", "/admin/v1/media/delete");
    ROUTE_REG("admin.v1.media.file",   "GET",  "/admin/v1/media/file/{name}");

    drogon::app().registerHandler("/admin/v1/media/list",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->list(req));
        },
        mediaGet);

    drogon::app().registerHandler("/admin/v1/media/upload",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->upload(req));
        },
        mediaPost);

    drogon::app().registerHandler("/admin/v1/media/delete",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->destroy(req));
        },
        mediaPost);

    drogon::app().registerHandler("/admin/v1/media/file/{name}",
        [ctrl](drogon::HttpRequestPtr req, std::string name)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->file(req, name);
        },
        mediaGet);
}
