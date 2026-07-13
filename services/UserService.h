#pragma once
#include "IUserService.h"
#include <drogon/orm/CoroMapper.h>

struct UserService : IUserService {
    drogon::Task<UserListResult> list(
        int page, int pageSize,
        const std::string &qName, const std::string &qEmail,
        const std::string &qStatus,
        const std::string &qCode = "",
        const std::string &qPhone = "") override;

    drogon::Task<drogon_model::cppadmin::Users> findById(
        const std::string &id) override;

    drogon::Task<drogon_model::cppadmin::Users> create(
        UserCreateInput input, const std::string &actorId) override;

    drogon::Task<drogon_model::cppadmin::Users> update(
        const std::string &id, UserUpdateInput input, const std::string &actorId) override;

    drogon::Task<drogon_model::cppadmin::Users> updateProfile(
        const std::string &id, ProfileUpdateInput input) override;

    drogon::Task<void> remove(const std::string &id) override;

    drogon::Task<std::vector<drogon_model::cppadmin::Roles>> rolesOf(
        const std::string &userId) override;

    drogon::Task<void> changePassword(
        const std::string &userId,
        const std::string &currentPassword,
        const std::string &newPassword) override;
};
