#pragma once
#include "IPermissionService.h"
#include <drogon/orm/CoroMapper.h>

struct PermissionService : IPermissionService {
    drogon::Task<PermissionListResult> list(
        int page, int pageSize,
        const std::string &qName, const std::string &qGuard,
        const std::string &qMethod, const std::string &qStatus) override;

    drogon::Task<std::vector<drogon_model::cppadmin::Permissions>> listAll() override;

    drogon::Task<drogon_model::cppadmin::Permissions> findById(
        const std::string &id) override;

    drogon::Task<drogon_model::cppadmin::Permissions> create(
        PermissionCreateInput input, const std::string &actorId) override;

    drogon::Task<drogon_model::cppadmin::Permissions> update(
        const std::string &id, PermissionUpdateInput input, const std::string &actorId) override;

    drogon::Task<void> remove(const std::string &id) override;
};
