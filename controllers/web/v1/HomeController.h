#pragma once
#include <drogon/drogon.h>
#include "../../../services/ISettingService.h"
#include <memory>

class HomeController {
public:
    explicit HomeController(std::shared_ptr<ISettingService> svc)
        : svc_(std::move(svc)) {}

    drogon::Task<drogon::HttpResponsePtr> index(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<ISettingService> svc_;
};
