#pragma once
#include <drogon/drogon.h>
#include "../../../services/ISettingService.h"
#include "../../../services/IFeTemplateService.h"
#include <memory>

class HomeController {
public:
    HomeController(std::shared_ptr<ISettingService> svc,
                   std::shared_ptr<IFeTemplateService> feTemplate)
        : svc_(std::move(svc)), feTemplate_(std::move(feTemplate)) {}

    drogon::Task<drogon::HttpResponsePtr> index(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<ISettingService>    svc_;
    std::shared_ptr<IFeTemplateService> feTemplate_;
};
