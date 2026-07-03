#include "DashboardService.h"
#include <drogon/drogon.h>

drogon::Task<DashboardStats> DashboardService::getStats(const std::string &userId) {
    auto db = drogon::app().getDbClient();

    auto rUsers = co_await db->execSqlCoro("SELECT COUNT(*) AS cnt FROM users");
    auto rRoles = co_await db->execSqlCoro("SELECT COUNT(*) AS cnt FROM roles");
    auto rPerms = co_await db->execSqlCoro("SELECT COUNT(*) AS cnt FROM permissions");

    DashboardStats stats;
    stats.totalUsers = rUsers[0]["cnt"].as<long long>();
    stats.totalRoles = rRoles[0]["cnt"].as<long long>();
    stats.totalPerms = rPerms[0]["cnt"].as<long long>();

    if (!userId.empty()) {
        auto rUser = co_await db->execSqlCoro(
            "SELECT name, email FROM users WHERE id=?", userId);
        if (!rUser.empty()) {
            stats.currentUserName  = rUser[0]["name"].as<std::string>();
            stats.currentUserEmail = rUser[0]["email"].as<std::string>();
        }
    }

    co_return stats;
}
