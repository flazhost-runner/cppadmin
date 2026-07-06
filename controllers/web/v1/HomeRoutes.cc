#include "HomeController.h"
#include "../../../services/SettingService.h"
#include "../../../services/FeTemplateService.h"
#include <drogon/drogon.h>
#include <memory>

void registerHomeRoutes() {
    auto feTemplate = std::make_shared<FeTemplateService>();
    auto ctrl = std::make_shared<HomeController>(
        std::make_shared<SettingService>(feTemplate), feTemplate);

    auto get = std::vector<drogon::internal::HttpConstraint>{drogon::Get};

    drogon::app().registerHandler("/",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->index(req));
        },
        get);

    drogon::app().registerHandler("/home",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
               -> drogon::Task<void> {
            cb(co_await ctrl->index(req));
        },
        get);
}
