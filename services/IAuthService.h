#pragma once
#include <drogon/drogon.h>
#include "../models/Users.h"
#include <string>
#include <memory>

struct LoginResult {
    drogon_model::cppadmin::Users user;
    std::string jwtToken;
};

struct RegisterInput {
    std::string name;
    std::string email;
    std::string password;
    std::string phone;
};

struct ResetRequestInput {
    std::string email;
};

struct ResetProcessInput {
    std::string email;
    std::string otp;
    std::string newPassword;
};

struct IAuthService {
    virtual ~IAuthService() = default;

    virtual drogon::Task<LoginResult> login(
        const std::string &email,
        const std::string &password) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Users> registerUser(
        RegisterInput input) = 0;

    virtual drogon::Task<void> requestPasswordReset(
        ResetRequestInput input) = 0;

    virtual drogon::Task<void> processPasswordReset(
        ResetProcessInput input) = 0;

    virtual drogon::Task<drogon_model::cppadmin::Users> findById(
        const std::string &id) = 0;
};
