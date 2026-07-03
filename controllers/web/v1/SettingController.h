#pragma once
#include <drogon/drogon.h>
#include "../../../services/ISettingService.h"
#include <memory>

class SettingController {
public:
    explicit SettingController(std::shared_ptr<ISettingService> svc)
        : svc_(std::move(svc)) {}

    drogon::Task<drogon::HttpResponsePtr> edit(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<ISettingService> svc_;
};
