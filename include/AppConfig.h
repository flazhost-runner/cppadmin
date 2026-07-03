#pragma once
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <trantor/utils/Logger.h>

// All runtime config & secrets — validated at startup.
// Use std::getenv() ONLY here; modules read AppConfig::instance().
// Secrets missing in production → LOG_FATAL + exit(1) (fail-fast).

struct AppConfig {
    std::string sessionSecret;
    std::string jwtSecret;
    std::string jwtExpiresIn{"1h"};   // string like '1h','30m','24h'
    std::string appMode;              // "full" | "api"
    std::string appEnv;               // "development" | "production" | "test"
    std::string appName;
    std::string appUrl;
    int         appPort{3000};
    int         sessionTtlHours{6};
    int         otpExpiryMinutes{10};
    int         defaultPageSize{10};

    // Storage (STORAGE_DRIVER: local | oss | s3)
    std::string storageDriver{"local"};
    std::string storageAccessKeyId;
    std::string storageSecretAccessKey;
    std::string storageEndpoint;
    std::string storageBucket;
    std::string storageRegion;
    bool        storageSsl{true};

    // Mail
    std::string mailHost;
    int         mailPort{587};
    bool        mailSecure{false};
    std::string mailUsername;
    std::string mailPassword;
    std::string mailFromName;
    std::string mailFromAddress;

    // Redis
    std::string redisUrl{"redis://127.0.0.1:6379"};

    int bcryptRounds{10};

    bool isProduction() const { return appEnv == "production"; }
    bool isTest()       const { return appEnv == "test"; }
    bool webUiEnabled() const { return appMode != "api"; }

    // Parse JWT_EXPIRES_IN string ('1h','30m','24h') to seconds
    long long jwtExpireSeconds() const {
        if (jwtExpiresIn.empty()) return 3600;
        char unit = jwtExpiresIn.back();
        long long n = std::stoll(jwtExpiresIn.substr(0, jwtExpiresIn.size() - 1));
        if (unit == 'h') return n * 3600;
        if (unit == 'm') return n * 60;
        if (unit == 'd') return n * 86400;
        return 3600; // default 1h
    }

    static AppConfig &instance() {
        static AppConfig cfg;
        return cfg;
    }

    // Call once at startup — throws std::runtime_error on fatal misconfiguration.
    void loadAndValidate() {
        auto env = [](const char *key, const char *def = "") -> std::string {
            const char *v = std::getenv(key);
            return v ? v : def;
        };
        auto envInt = [](const char *key, int def) -> int {
            const char *v = std::getenv(key);
            return v ? std::stoi(v) : def;
        };
        auto envBool = [](const char *key, bool def) -> bool {
            const char *v = std::getenv(key);
            if (!v) return def;
            std::string s(v);
            return s == "true" || s == "1" || s == "yes";
        };

        appEnv           = env("APP_ENV", "development");
        appMode          = env("APP_MODE", "full");
        appName          = env("APP_NAME", "CppAdmin");
        appUrl           = env("APP_URL", "http://localhost:3000");
        appPort          = envInt("APP_PORT", 3000);
        sessionSecret    = env("SESSION_SECRET");
        sessionTtlHours  = envInt("SESSION_TTL_HOURS", 6);
        jwtSecret        = env("JWT_SECRET");
        jwtExpiresIn     = env("JWT_EXPIRES_IN", "1h");
        bcryptRounds     = envInt("BCRYPT_ROUNDS", 10);
        otpExpiryMinutes = envInt("OTP_EXPIRY_MINUTES", 10);
        defaultPageSize  = envInt("DEFAULT_PAGE_SIZE", 10);

        // Storage
        storageDriver          = env("STORAGE_DRIVER", "local");
        storageAccessKeyId     = env("STORAGE_ACCESS_KEY_ID");
        storageSecretAccessKey = env("STORAGE_SECRET_ACCESS_KEY");
        storageEndpoint        = env("STORAGE_ENDPOINT");
        storageBucket          = env("STORAGE_BUCKET");
        storageRegion          = env("STORAGE_REGION");
        storageSsl             = envBool("STORAGE_SSL", true);

        // Mail
        mailHost        = env("MAIL_HOST");
        mailPort        = envInt("MAIL_PORT", 587);
        mailSecure      = envBool("MAIL_SECURE", false);
        mailUsername    = env("MAIL_USERNAME");
        mailPassword    = env("MAIL_PASSWORD");
        mailFromName    = env("MAIL_FROM_NAME", "CppAdmin");
        mailFromAddress = env("MAIL_FROM_ADDRESS");

        // Redis
        redisUrl = env("REDIS_URL", "redis://127.0.0.1:6379");

        if (isProduction()) {
            if (sessionSecret.empty()) {
                LOG_FATAL << "SESSION_SECRET must be set in production";
                exit(1);
            }
            if (jwtSecret.empty()) {
                LOG_FATAL << "JWT_SECRET must be set in production";
                exit(1);
            }
        } else {
            if (sessionSecret.empty()) sessionSecret = "dev-session-secret-change-me";
            if (jwtSecret.empty())     jwtSecret     = "dev-jwt-secret-change-me";
        }
    }

private:
    AppConfig() = default;
};
