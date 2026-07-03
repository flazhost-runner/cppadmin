#pragma once
#include "../models/Permissions.h"
#include <drogon/drogon.h>
#include <string>
#include <vector>

struct PermissionCreateInput {
    std::string name;
    std::string guardName{"web"};
    std::string method{"GET"};
    std::string status{"Active"};
    std::string desc;
};

struct PermissionUpdateInput {
    std::string name;
    std::string guardName;
    std::string method;
    std::string status;
    std::string desc;
};

struct PermissionListResult {
    std::vector<drogon_model::cppadmin::Permissions> rows;
    long long total{0};
};

struct IPermissionService {
    virtual ~IPermissionService() = default;

    virtual drogon::Task<PermissionListResult> list(
        int page, int pageSize,
        const std::string &qName, const std::string &qGuard,
        const std::string &qMethod, const std::string &qStatus) = 0;

    virtual drogon::Task<std::vector<drogon_model::cppadmin::Permissions>> listAll() = 0;

    virtual drogon::Task<drogon_model::cppadmin::Permissions> findById(
        const std::string &id) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Permissions> create(
        PermissionCreateInput input, const std::string &actorId) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Permissions> update(
        const std::string &id, PermissionUpdateInput input, const std::string &actorId) = 0;

    virtual drogon::Task<void> remove(const std::string &id) = 0;
};
