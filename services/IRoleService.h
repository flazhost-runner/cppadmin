#pragma once
#include "../models/Roles.h"
#include "../models/Permissions.h"
#include <drogon/drogon.h>
#include <string>
#include <vector>

struct RoleCreateInput {
    std::string name;
    std::string guardName{"web"};
    std::string status{"Active"};
    std::string desc;
    std::vector<std::string> permissionIds;
};

struct RoleUpdateInput {
    std::string name;
    std::string guardName;
    std::string status;
    std::string desc;
    std::vector<std::string> permissionIds;
};

struct RoleListResult {
    std::vector<drogon_model::cppadmin::Roles> rows;
    long long total{0};
};

struct IRoleService {
    virtual ~IRoleService() = default;

    virtual drogon::Task<RoleListResult> list(
        int page, int pageSize,
        const std::string &qName,
        const std::string &qStatus) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Roles> findById(
        const std::string &id) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Roles> create(
        RoleCreateInput input, const std::string &actorId) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Roles> update(
        const std::string &id, RoleUpdateInput input, const std::string &actorId) = 0;

    virtual drogon::Task<void> remove(const std::string &id) = 0;

    virtual drogon::Task<std::vector<drogon_model::cppadmin::Permissions>> permissionsOf(
        const std::string &roleId) = 0;

    virtual drogon::Task<void> assignPermission(
        const std::string &roleId, const std::string &permissionId) = 0;

    virtual drogon::Task<void> unassignPermission(
        const std::string &roleId, const std::string &permissionId) = 0;

    virtual drogon::Task<void> assignPermissions(
        const std::string &roleId, const std::vector<std::string> &permissionIds) = 0;

    virtual drogon::Task<void> unassignPermissions(
        const std::string &roleId, const std::vector<std::string> &permissionIds) = 0;
};
