#include "AuthService.h"
#include "../include/AppError.h"
#include "../include/AppConfig.h"
#include "../include/helpers/UuidGen.h"
#include "../include/helpers/JwtHelper.h"
#include "../vendor/bcrypt/bcrypt.h"
#include <drogon/drogon.h>
#include <drogon/orm/CoroMapper.h>
#include <openssl/rand.h>
#include <chrono>
#include <sstream>
#include <iomanip>

using namespace drogon_model::cppadmin;
using drogon::orm::CoroMapper;
using drogon::orm::Criteria;
using drogon::orm::CompareOperator;
using CompareOp = drogon::orm::CompareOperator;

// ── JWT ────────────────────────────────────────────────────────────────────────
std::string AuthService::issueJwt(const std::string &userId, const std::string &email) const {
    auto &cfg = AppConfig::instance();
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    Json::Value payload;
    payload["sub"]   = userId;
    payload["email"] = email;
    payload["iat"]   = (Json::Int64)now;
    payload["exp"]   = (Json::Int64)(now + cfg.jwtExpireSeconds());

    return jwt_helper::sign(payload, cfg.jwtSecret);
}

// ── OTP generator ─────────────────────────────────────────────────────────────
std::string AuthService::generateOtp(int digits) {
    unsigned int rnd = 0;
    RAND_bytes(reinterpret_cast<unsigned char *>(&rnd), sizeof(rnd));
    int mod = 1;
    for (int i = 0; i < digits; i++) mod *= 10;
    int n = static_cast<int>(rnd % static_cast<unsigned int>(mod));
    std::ostringstream ss;
    ss << std::setw(digits) << std::setfill('0') << n;
    return ss.str();
}

// ── login ──────────────────────────────────────────────────────────────────────
drogon::Task<LoginResult> AuthService::login(const std::string &email, const std::string &password) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);
    std::vector<Users> rows;
    try {
        rows = co_await mapper.findBy(
            Criteria(Users::Cols::_email, CompareOperator::EQ, email));
    } catch (...) {
        rows.clear();
    }
    if (rows.empty())
        throw UnauthorizedError("Wrong email or password.");

    const Users &u = rows.front();
    if (!bcrypt::verify(password, u.getValueOfPassword()))
        throw UnauthorizedError("Wrong email or password.");

    if (u.getValueOfStatus() != "Active")
        throw UnauthorizedError("Account is inactive or blocked.");

    if (u.getValueOfBlocked())
        throw UnauthorizedError("Account is blocked.");

    co_return LoginResult{u, issueJwt(u.getValueOfId(), u.getValueOfEmail())};
}

// ── registerUser ───────────────────────────────────────────────────────────────
drogon::Task<Users> AuthService::registerUser(RegisterInput input) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    auto existing = co_await mapper.findBy(
        Criteria(Users::Cols::_email, CompareOperator::EQ, input.email));
    if (!existing.empty())
        throw ConflictError("Email already exists.");

    Users u;
    u.setId(newUuid());
    u.setCode("USR-" + newUuid().substr(0, 8));
    u.setName(input.name);
    u.setEmail(input.email);
    u.setPhone(input.phone);
    u.setPassword(bcrypt::hash(input.password));
    u.setStatus("active");
    u.setTimezone("UTC");
    u.setBlocked(false);
    u.setCreatedBy("system");
    u.setUpdatedBy("system");

    co_await mapper.insert(u);
    co_return u;
}

// ── requestPasswordReset ───────────────────────────────────────────────────────
drogon::Task<void> AuthService::requestPasswordReset(ResetRequestInput input) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    auto rows = co_await mapper.findBy(
        Criteria(Users::Cols::_email, CompareOperator::EQ, input.email));
    if (rows.empty())
        co_return; // silent — don't reveal existence

    Users u = rows.front();
    std::string otp = generateOtp(6);
    auto &cfg = AppConfig::instance();
    long long expSec = (long long)cfg.otpExpiryMinutes * 60;
    long long exp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() + expSec;

    u.setPasswordOtp(bcrypt::hash(otp, cfg.bcryptRounds));
    u.setPasswordOtpExpires(exp);
    u.setUpdatedBy("system");
    co_await mapper.update(u);

    // TODO: send OTP via email (SMTP not wired in Phase 1)
    LOG_INFO << "OTP for " << input.email << ": " << otp;
    co_return;
}

// ── processPasswordReset ───────────────────────────────────────────────────────
drogon::Task<void> AuthService::processPasswordReset(ResetProcessInput input) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);

    auto rows = co_await mapper.findBy(
        Criteria(Users::Cols::_email, CompareOperator::EQ, input.email));
    if (rows.empty())
        throw ValidationError("Email not found.");

    Users u = rows.front();
    const long long *exp = u.getPasswordOtpExpires();
    const std::string *storedOtp = u.getPasswordOtp();

    if (!storedOtp || storedOtp->empty() || !bcrypt::verify(input.otp, *storedOtp))
        throw ValidationError("OTP is invalid.");

    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    if (!exp || *exp < now)
        throw ValidationError("OTP has expired.");

    u.setPassword(bcrypt::hash(input.newPassword));
    u.setPasswordOtpToNull();
    u.setPasswordOtpExpiresToNull();
    u.setUpdatedBy("system");
    co_await mapper.update(u);
    co_return;
}

// ── findById ───────────────────────────────────────────────────────────────────
drogon::Task<Users> AuthService::findById(const std::string &id) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Users> mapper(db);
    try {
        co_return co_await mapper.findByPrimaryKey(id);
    } catch (const drogon::orm::UnexpectedRows &) {
        throw NotFoundError("User not found.");
    }
}
