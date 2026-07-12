#include "UserService.h"
#include "../include/AppError.h"
#include "../include/helpers/UuidGen.h"
#include "../vendor/bcrypt/bcrypt.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>

using namespace drogon_model::cppadmin;
using drogon::orm::CoroMapper;
using drogon::orm::Criteria;
using drogon::orm::CompareOperator;
using CompareOp = drogon::orm::CompareOperator;

drogon::Task<UserListResult> UserService::list(
        int page, int pageSize,
        const std::string &qName, const std::string &qEmail,
        const std::string &qStatus,
        const std::string &qCode,
        const std::string &qPhone) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    Criteria crit;
    bool hasCrit = false;

    auto addAnd = [&](Criteria c) {
        if (!hasCrit) { crit = c; hasCrit = true; }
        else           crit = crit && c;
    };
    if (!qCode.empty())
        addAnd(Criteria(Users::Cols::_code, CompareOperator::Like, "%" + qCode + "%"));
    if (!qName.empty())
        addAnd(Criteria(Users::Cols::_name, CompareOperator::Like, "%" + qName + "%"));
    if (!qPhone.empty())
        addAnd(Criteria(Users::Cols::_phone, CompareOperator::Like, "%" + qPhone + "%"));
    if (!qEmail.empty())
        addAnd(Criteria(Users::Cols::_email, CompareOperator::Like, "%" + qEmail + "%"));
    if (!qStatus.empty())
        addAnd(Criteria(Users::Cols::_status, CompareOperator::EQ, qStatus));

    long long total = 0;
    std::vector<Users> rows;

    if (hasCrit) {
        total = co_await mapper.count(crit);
        rows  = co_await mapper.orderBy(Users::Cols::_name, drogon::orm::SortOrder::ASC)
                               .paginate(page, pageSize)
                               .findBy(crit);
    } else {
        total = co_await mapper.count();
        rows  = co_await mapper.orderBy(Users::Cols::_name, drogon::orm::SortOrder::ASC)
                               .paginate(page, pageSize)
                               .findAll();
    }

    co_return UserListResult{std::move(rows), total};
}

drogon::Task<Users> UserService::findById(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);
    try {
        co_return co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("User not found.");
    }
}

drogon::Task<Users> UserService::create(UserCreateInput input, const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    // Check duplicate email
    auto existing = co_await mapper.findBy(
        Criteria(Users::Cols::_email, CompareOperator::EQ, input.email));
    if (!existing.empty())
        throw ConflictError("Email already in use.");

    // Check duplicate code
    auto existingCode = co_await mapper.findBy(
        Criteria(Users::Cols::_code, CompareOperator::EQ, input.code));
    if (!existingCode.empty())
        throw ConflictError("User code already in use.");

    Users u;
    u.setId(newUuid());
    u.setCode(input.code);
    u.setName(input.name);
    u.setEmail(input.email);
    u.setPhone(input.phone);
    u.setPassword(bcrypt::hash(input.password));
    u.setStatus(input.status.empty() ? "Active" : input.status);
    u.setTimezone(input.timezone.empty() ? "UTC" : input.timezone);
    if (!input.picture.empty()) u.setPicture(input.picture);
    u.setBlocked(input.blocked);
    if (input.blocked && !input.blockedReason.empty()) u.setBlockedReason(input.blockedReason);
    u.setCreatedBy(actorId);
    u.setUpdatedBy(actorId);

    co_await mapper.insert(u);

    // Assign roles
    if (!input.roleIds.empty()) {
        for (const auto &roleId : input.roleIds) {
            co_await db->execSqlCoro(
                "INSERT OR IGNORE INTO users_roles (user_id, role_id) VALUES (?, ?)",
                u.getValueOfId(), roleId);
        }
    }

    co_return u;
}

drogon::Task<Users> UserService::update(
        const std::string &id, UserUpdateInput input, const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    Users u;
    try {
        u = co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("User not found.");
    }

    if (!input.name.empty())     u.setName(input.name);
    if (!input.phone.empty())    u.setPhone(input.phone);
    if (!input.status.empty())   u.setStatus(input.status);
    if (!input.timezone.empty()) u.setTimezone(input.timezone);
    if (!input.picture.empty())  u.setPicture(input.picture);
    u.setBlocked(input.blocked);
    if (input.blocked && !input.blockedReason.empty()) u.setBlockedReason(input.blockedReason);
    u.setUpdatedBy(actorId);

    co_await mapper.update(u);

    // Replace roles
    if (!input.roleIds.empty()) {
        co_await db->execSqlCoro("DELETE FROM users_roles WHERE user_id = ?", id);
        for (const auto &roleId : input.roleIds) {
            co_await db->execSqlCoro(
                "INSERT OR IGNORE INTO users_roles (user_id, role_id) VALUES (?, ?)",
                id, roleId);
        }
    }

    co_return u;
}

drogon::Task<void> UserService::remove(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);
    try {
        co_await mapper.findByPrimaryKey(id); // verify exists
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("User not found.");
    }
    co_await mapper.deleteByPrimaryKey(id);
}

drogon::Task<std::vector<Roles>> UserService::rolesOf(const std::string &userId) {
    auto db = drogon::app().getDbClient();
    auto result = co_await db->execSqlCoro(
        "SELECT r.* FROM roles r "
        "INNER JOIN users_roles ur ON ur.role_id = r.id "
        "WHERE ur.user_id = ?",
        userId);

    std::vector<Roles> roles;
    for (const auto &row : result) {
        roles.emplace_back(Roles(row));
    }
    co_return roles;
}

drogon::Task<void> UserService::changePassword(
        const std::string &userId,
        const std::string &currentPassword,
        const std::string &newPassword) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    Users u;
    try {
        u = co_await mapper.findByPrimaryKey(userId);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("User not found.");
    }

    if (!bcrypt::verify(currentPassword, u.getValueOfPassword())) {
        throw ValidationError("Current password is incorrect.");
    }

    std::string hashed = bcrypt::hash(newPassword);
    co_await db->execSqlCoro("UPDATE users SET password=? WHERE id=?", hashed, userId);
}
