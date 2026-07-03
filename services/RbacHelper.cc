#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include "../models/Roles.h"
#include "../models/Permissions.h"

using namespace drogon_model::cppadmin;
using drogon::orm::CoroMapper;

// Is userId a member of the 'Administrator' role?
drogon::Task<bool> userIsAdministratorAsync(const std::string &userId) {
    auto db = drogon::app().getDbClient();
    auto result = co_await db->execSqlCoro(
        "SELECT 1 FROM users_roles ur "
        "INNER JOIN roles r ON r.id = ur.role_id "
        "WHERE ur.user_id = ? AND r.name = 'Administrator' "
        "LIMIT 1",
        userId);
    co_return !result.empty();
}

// Does userId have a permission with name = routeName?
drogon::Task<bool> userHasAccessAsync(const std::string &userId,
                                       const std::string &routeName,
                                       const std::string & /*method*/) {
    auto db = drogon::app().getDbClient();
    auto result = co_await db->execSqlCoro(
        "SELECT 1 FROM users_roles ur "
        "INNER JOIN roles_permissions rp ON rp.role_id = ur.role_id "
        "INNER JOIN permissions p ON p.id = rp.permission_id "
        "WHERE ur.user_id = ? AND p.name = ? AND p.status = 'Active' "
        "LIMIT 1",
        userId, routeName);
    co_return !result.empty();
}
