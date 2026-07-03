#pragma once
#include <drogon/drogon.h>
#include "../../../services/IDashboardService.h"
#include <memory>

class DashboardController {
public:
    explicit DashboardController(std::shared_ptr<IDashboardService> svc)
        : svc_(std::move(svc)) {}

    drogon::Task<drogon::HttpResponsePtr> index(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IDashboardService> svc_;
};
