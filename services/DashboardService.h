#pragma once
#include "IDashboardService.h"

struct DashboardService : IDashboardService {
    drogon::Task<DashboardStats> getStats(const std::string &userId) override;
};
