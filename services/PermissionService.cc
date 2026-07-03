#include "PermissionService.h"
#include "../include/AppError.h"
#include "../include/helpers/UuidGen.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

using namespace drogon_model::cppadmin;
using drogon::orm::CoroMapper;
using drogon::orm::Criteria;
using drogon::orm::CompareOperator;
using CompareOp = drogon::orm::CompareOperator;

drogon::Task<PermissionListResult> PermissionService::list(
        int page, int pageSize,
        const std::string &qName, const std::string &qGuard,
        const std::string &qMethod, const std::string &qStatus) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Permissions> mapper(db);

    Criteria crit;
    bool hasCrit = false;

    auto addAnd = [&](Criteria c) {
        if (!hasCrit) { crit = c; hasCrit = true; }
        else           crit = crit && c;
    };
    if (!qName.empty())
        addAnd(Criteria(Permissions::Cols::_name, CompareOperator::Like, "%" + qName + "%"));
    if (!qGuard.empty())
        addAnd(Criteria(Permissions::Cols::_guard_name, CompareOperator::EQ, qGuard));
    if (!qMethod.empty())
        addAnd(Criteria(Permissions::Cols::_method, CompareOperator::EQ, qMethod));
    if (!qStatus.empty())
        addAnd(Criteria(Permissions::Cols::_status, CompareOperator::EQ, qStatus));

    long long total = 0;
    std::vector<Permissions> rows;

    if (hasCrit) {
        total = co_await mapper.count(crit);
        rows  = co_await mapper.orderBy(Permissions::Cols::_name, drogon::orm::SortOrder::ASC)
                               .paginate(page, pageSize)
                               .findBy(crit);
    } else {
        total = co_await mapper.count();
        rows  = co_await mapper.orderBy(Permissions::Cols::_name, drogon::orm::SortOrder::ASC)
                               .paginate(page, pageSize)
                               .findAll();
    }

    co_return PermissionListResult{std::move(rows), total};
}

drogon::Task<std::vector<Permissions>> PermissionService::listAll() {
    auto db = drogon::app().getDbClient();
    CoroMapper<Permissions> mapper(db);
    co_return co_await mapper.orderBy(Permissions::Cols::_name, drogon::orm::SortOrder::ASC).findAll();
}

drogon::Task<Permissions> PermissionService::findById(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Permissions> mapper(db);
    try {
        co_return co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("Permission not found.");
    }
}

drogon::Task<Permissions> PermissionService::create(
        PermissionCreateInput input, const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Permissions> mapper(db);

    Permissions p;
    p.setId(newUuid());
    p.setName(input.name);
    p.setGuardName(input.guardName.empty() ? "web" : input.guardName);
    p.setMethod(input.method.empty() ? "GET" : input.method);
    p.setStatus(input.status.empty() ? "Active" : input.status);
    if (!input.desc.empty()) p.setDesc(input.desc);
    p.setCreatedBy(actorId);
    p.setUpdatedBy(actorId);

    co_await mapper.insert(p);
    co_return p;
}

drogon::Task<Permissions> PermissionService::update(
        const std::string &id, PermissionUpdateInput input, const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Permissions> mapper(db);

    Permissions p;
    try {
        p = co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("Permission not found.");
    }

    if (!input.name.empty())      p.setName(input.name);
    if (!input.guardName.empty()) p.setGuardName(input.guardName);
    if (!input.method.empty())    p.setMethod(input.method);
    if (!input.status.empty())    p.setStatus(input.status);
    if (!input.desc.empty())      p.setDesc(input.desc);
    p.setUpdatedBy(actorId);

    co_await mapper.update(p);
    co_return p;
}

drogon::Task<void> PermissionService::remove(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Permissions> mapper(db);
    try {
        co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("Permission not found.");
    }
    co_await mapper.deleteByPrimaryKey(id);
}
