#pragma once
#include <drogon/drogon.h>
#include <drogon/utils/coroutine.h>
#include <string>

// JWT blacklist backed by the `jwt_blacklist` table in the main DB.
// Used to invalidate web JWT cookies on explicit logout before natural expiry.
namespace JwtBlacklist {

// Record a revoked JTI until its natural expiry (unix epoch seconds).
inline drogon::Task<void> blacklist(const std::string &jti, long long expiresUnix) {
    auto db = drogon::app().getDbClient();
    co_await db->execSqlCoro(
        "INSERT OR IGNORE INTO jwt_blacklist(jti, expires_at) "
        "VALUES(?, datetime(?, 'unixepoch'))",
        jti, std::to_string(expiresUnix));
}

// Return true if the JTI is revoked and the entry has not yet expired.
inline drogon::Task<bool> isBlacklisted(const std::string &jti) {
    auto db = drogon::app().getDbClient();
    auto result = co_await db->execSqlCoro(
        "SELECT 1 FROM jwt_blacklist WHERE jti=? "
        "AND expires_at > datetime('now') LIMIT 1",
        jti);
    co_return !result.empty();
}

} // namespace JwtBlacklist
