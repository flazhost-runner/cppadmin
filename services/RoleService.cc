#include "RoleService.h"
#include "../include/AppError.h"
#include "../include/helpers/UuidGen.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

using namespace drogon_model::cppadmin;
using drogon::orm::CoroMapper;
using drogon::orm::Criteria;
using drogon::orm::CompareOperator;
using CompareOp = drogon::orm::CompareOperator;

drogon::Task<RoleListResult> RoleService::list(
        int page, int pageSize,
        const std::string &qName,
        const std::string &qStatus) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Roles> mapper(db);

    Criteria crit;
    bool hasCrit = false;

    auto addAnd = [&](Criteria c) {
        if (!hasCrit) { crit = c; hasCrit = true; }
        else           crit = crit && c;
    };
    if (!qName.empty())
        addAnd(Criteria(Roles::Cols::_name, CompareOperator::Like, "%" + qName + "%"));
    if (!qStatus.empty())
        addAnd(Criteria(Roles::Cols::_status, CompareOperator::EQ, qStatus));

    long long total = 0;
    std::vector<Roles> rows;

    if (hasCrit) {
        total = co_await mapper.count(crit);
        rows  = co_await mapper.orderBy(Roles::Cols::_name, drogon::orm::SortOrder::ASC)
                               .paginate(page, pageSize)
                               .findBy(crit);
    } else {
        total = co_await mapper.count();
        rows  = co_await mapper.orderBy(Roles::Cols::_name, drogon::orm::SortOrder::ASC)
                               .paginate(page, pageSize)
                               .findAll();
    }

    co_return RoleListResult{std::move(rows), total};
}

drogon::Task<Roles> RoleService::findById(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Roles> mapper(db);
    try {
        co_return co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("Role not found.");
    }
}

drogon::Task<Roles> RoleService::create(RoleCreateInput input, const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Roles> mapper(db);

    auto existing = co_await mapper.findBy(
        Criteria(Roles::Cols::_name, CompareOperator::EQ, input.name));
    if (!existing.empty())
        throw ConflictError("Role name already in use.");

    Roles r;
    r.setId(newUuid());
    r.setName(input.name);
    r.setGuardName(input.guardName.empty() ? "web" : input.guardName);
    r.setStatus(input.status.empty() ? "Active" : input.status);
    if (!input.desc.empty()) r.setDesc(input.desc);
    r.setCreatedBy(actorId);
    r.setUpdatedBy(actorId);

    co_await mapper.insert(r);

    for (const auto &permId : input.permissionIds) {
        co_await db->execSqlCoro(
            "INSERT OR IGNORE INTO roles_permissions (role_id, permission_id) VALUES (?, ?)",
            r.getValueOfId(), permId);
    }

    co_return r;
}

drogon::Task<Roles> RoleService::update(
        const std::string &id, RoleUpdateInput input, const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Roles> mapper(db);

    Roles r;
    try {
        r = co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("Role not found.");
    }

    // Check name uniqueness only if name changed
    if (!input.name.empty() && input.name != r.getValueOfName()) {
        auto dupe = co_await mapper.findBy(
            Criteria(Roles::Cols::_name, CompareOperator::EQ, input.name));
        if (!dupe.empty())
            throw ConflictError("Role name already in use.");
        r.setName(input.name);
    }
    if (!input.guardName.empty()) r.setGuardName(input.guardName);
    if (!input.status.empty())    r.setStatus(input.status);
    if (!input.desc.empty())      r.setDesc(input.desc);
    r.setUpdatedBy(actorId);

    co_await mapper.update(r);

    if (!input.permissionIds.empty()) {
        co_await db->execSqlCoro("DELETE FROM roles_permissions WHERE role_id = ?", id);
        for (const auto &permId : input.permissionIds) {
            co_await db->execSqlCoro(
                "INSERT OR IGNORE INTO roles_permissions (role_id, permission_id) VALUES (?, ?)",
                id, permId);
        }
    }

    co_return r;
}

drogon::Task<void> RoleService::remove(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Roles> mapper(db);
    try {
        co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("Role not found.");
    }
    co_await mapper.deleteByPrimaryKey(id);
}

drogon::Task<std::vector<Permissions>> RoleService::permissionsOf(const std::string &roleId) {
    auto db = drogon::app().getDbClient();
    auto result = co_await db->execSqlCoro(
        "SELECT p.* FROM permissions p "
        "INNER JOIN roles_permissions rp ON rp.permission_id = p.id "
        "WHERE rp.role_id = ?",
        roleId);

    std::vector<Permissions> perms;
    for (const auto &row : result) {
        perms.emplace_back(Permissions(row));
    }
    co_return perms;
}

drogon::Task<void> RoleService::assignPermission(const std::string &roleId,
                                                  const std::string &permissionId) {
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro(
        "INSERT OR IGNORE INTO roles_permissions (role_id, permission_id) VALUES (?, ?)",
        roleId, permissionId);
}

drogon::Task<void> RoleService::unassignPermission(const std::string &roleId,
                                                    const std::string &permissionId) {
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro(
        "DELETE FROM roles_permissions WHERE role_id = ? AND permission_id = ?",
        roleId, permissionId);
}

drogon::Task<void> RoleService::assignPermissions(const std::string &roleId,
                                                   const std::vector<std::string> &permissionIds) {
    auto db = drogon::app().getDbClient();
    for (const auto &pid : permissionIds) {
        co_await db->execSqlCoro(
            "INSERT OR IGNORE INTO roles_permissions (role_id, permission_id) VALUES (?, ?)",
            roleId, pid);
    }
}

drogon::Task<void> RoleService::unassignPermissions(const std::string &roleId,
                                                     const std::vector<std::string> &permissionIds) {
    auto db = drogon::app().getDbClient();
    for (const auto &pid : permissionIds) {
        co_await db->execSqlCoro(
            "DELETE FROM roles_permissions WHERE role_id = ? AND permission_id = ?",
            roleId, pid);
    }
}
