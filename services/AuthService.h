#pragma once
#include "IAuthService.h"
#include <drogon/orm/CoroMapper.h>
#include <memory>

struct AuthService : IAuthService {
    explicit AuthService() = default;

    drogon::Task<LoginResult> login(
        const std::string &email,
        const std::string &password) override;

    drogon::Task<drogon_model::cppadmin::Users> registerUser(
        RegisterInput input) override;

    drogon::Task<void> requestPasswordReset(
        ResetRequestInput input) override;

    drogon::Task<void> processPasswordReset(
        ResetProcessInput input) override;

    drogon::Task<drogon_model::cppadmin::Users> findById(
        const std::string &id) override;

private:
    std::string issueJwt(const std::string &userId, const std::string &email) const;
    static std::string generateOtp(int digits = 6);
};
