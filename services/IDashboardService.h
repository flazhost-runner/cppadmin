#pragma once
#include <drogon/drogon.h>
#include <string>

struct DashboardStats {
    long long totalUsers{0};
    long long totalRoles{0};
    long long totalPerms{0};
    std::string currentUserName;
    std::string currentUserEmail;
};

struct IDashboardService {
    virtual ~IDashboardService() = default;

    virtual drogon::Task<DashboardStats> getStats(const std::string &userId) = 0;
};
