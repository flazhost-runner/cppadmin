#pragma once
#include "IRoleService.h"
#include <drogon/orm/CoroMapper.h>

struct RoleService : IRoleService {
    drogon::Task<RoleListResult> list(
        int page, int pageSize,
        const std::string &qName,
        const std::string &qStatus) override;

    drogon::Task<drogon_model::cppadmin::Roles> findById(
        const std::string &id) override;

    drogon::Task<drogon_model::cppadmin::Roles> create(
        RoleCreateInput input, const std::string &actorId) override;

    drogon::Task<drogon_model::cppadmin::Roles> update(
        const std::string &id, RoleUpdateInput input, const std::string &actorId) override;

    drogon::Task<void> remove(const std::string &id) override;

    drogon::Task<std::vector<drogon_model::cppadmin::Permissions>> permissionsOf(
        const std::string &roleId) override;

    drogon::Task<void> assignPermission(
        const std::string &roleId, const std::string &permissionId) override;

    drogon::Task<void> unassignPermission(
        const std::string &roleId, const std::string &permissionId) override;

    drogon::Task<void> assignPermissions(
        const std::string &roleId, const std::vector<std::string> &permissionIds) override;

    drogon::Task<void> unassignPermissions(
        const std::string &roleId, const std::vector<std::string> &permissionIds) override;
};
