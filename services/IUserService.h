#pragma once
#include "../models/Users.h"
#include "../models/Roles.h"
#include <drogon/drogon.h>
#include <string>
#include <vector>

struct UserCreateInput {
    std::string code;
    std::string name;
    std::string email;
    std::string phone;
    std::string password;
    std::string status{"Active"};
    std::string timezone{"UTC"};
    std::string picture;   // KEY objek storage; "" bila tidak ada unggahan
    bool        blocked{false};
    std::string blockedReason;
    std::vector<std::string> roleIds;
};

struct UserUpdateInput {
    std::string name;
    std::string phone;
    std::string status;
    std::string timezone;
    std::string picture;
    bool        blocked{false};
    std::string blockedReason;
    std::vector<std::string> roleIds;
};

// Self-service profile edit (mirror NodeAdmin UserService.updateProfile): satu form
// penuh code/name/email/phone/timezone/status + password & picture opsional. Tidak
// menyentuh roles. Field kosong dilewati (foto & password lama tetap utuh).
struct ProfileUpdateInput {
    std::string code;
    std::string name;
    std::string email;
    std::string phone;
    std::string timezone;
    std::string status;
    std::string password;   // plaintext; "" = tak diubah
    std::string picture;    // KEY objek storage; "" = tak diubah
};

struct UserListResult {
    std::vector<drogon_model::cppadmin::Users> rows;
    long long total{0};
};

struct IUserService {
    virtual ~IUserService() = default;

    virtual drogon::Task<UserListResult> list(
        int page, int pageSize,
        const std::string &qName, const std::string &qEmail,
        const std::string &qStatus,
        const std::string &qCode = "",
        const std::string &qPhone = "") = 0;

    virtual drogon::Task<drogon_model::cppadmin::Users> findById(
        const std::string &id) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Users> create(
        UserCreateInput input, const std::string &actorId) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Users> update(
        const std::string &id, UserUpdateInput input, const std::string &actorId) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Users> updateProfile(
        const std::string &id, ProfileUpdateInput input) = 0;

    virtual drogon::Task<void> remove(
        const std::string &id) = 0;

    virtual drogon::Task<std::vector<drogon_model::cppadmin::Roles>> rolesOf(
        const std::string &userId) = 0;

    // Throws ValidationError if currentPassword is wrong.
    virtual drogon::Task<void> changePassword(
        const std::string &userId,
        const std::string &currentPassword,
        const std::string &newPassword) = 0;
};
