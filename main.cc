#include <drogon/drogon.h>
#include <csignal>
#include <atomic>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include "include/AppConfig.h"
#include "include/AppError.h"
#include "include/RouteRegistry.h"

// Forward declarations — each module registers its routes at startup.
void registerAuthRoutes();
void registerAccessRoutes();
void registerDashboardRoutes();
void registerComponentsRoutes();
void registerSettingRoutes();
void registerProfileRoutes();
void registerHomeRoutes();
void registerMediaRoutes();
void registerStorageRoutes();

// Dev-only: auto-migrate + seed so `run` works immediately without manual setup.
static void autoMigrateAndSeed(const std::string &appEnv,
                               const std::filesystem::path &appRoot) {
    if (appEnv == "production") return;

    if (std::system("which dbmate >/dev/null 2>&1") != 0) {
        LOG_WARN << "Auto-migrate: skipped — dbmate not found in PATH";
        return;
    }

    // dbmate needs DATABASE_URL. Default to the same sqlite DB that
    // config.json db_clients uses (./dev.db, relative to CWD) so a fresh
    // dev run works with zero configuration.
    const char *urlEnv = std::getenv("DATABASE_URL");
    std::string databaseUrl = (urlEnv && *urlEnv) ? urlEnv : "sqlite:./dev.db";
    if (!urlEnv || !*urlEnv) {
        setenv("DATABASE_URL", databaseUrl.c_str(), 1);
        LOG_INFO << "Auto-migrate: DATABASE_URL not set — defaulting to "
                 << databaseUrl;
    }

    // dbmate up (non-interactive) — migrations live in db/migrations/.
    // Absolute migrations dir so this works regardless of CWD; no schema
    // dump so dev runs never rewrite db/schema.sql.
    auto migrationsDir = appRoot / "db" / "migrations";
    std::string upCmd = "dbmate --no-dump-schema --migrations-dir \"" +
                        migrationsDir.string() + "\" up 2>&1";
    if (std::system(upCmd.c_str()) != 0) {
        LOG_ERROR << "Auto-migrate: dbmate up FAILED (DATABASE_URL="
                  << databaseUrl << ") — seed skipped; fix the URL or run "
                  << "migrations manually";
        return;
    }
    LOG_INFO << "Auto-migrate: done (appEnv=" << appEnv << ")";

    // Idempotent dev seed (admin@admin.com / 12345678) — sqlite only.
    if (databaseUrl.rfind("sqlite:", 0) != 0) {
        LOG_INFO << "Auto-seed: skipped (non-sqlite DATABASE_URL)";
        return;
    }
    auto seedFile = appRoot / "db" / "seeds" / "seed.sql";
    if (!std::filesystem::exists(seedFile)) {
        LOG_WARN << "Auto-seed: skipped — " << seedFile.string() << " not found";
        return;
    }
    if (std::system("which sqlite3 >/dev/null 2>&1") != 0) {
        LOG_WARN << "Auto-seed: skipped — sqlite3 CLI not found in PATH";
        return;
    }
    std::string dbFile = databaseUrl.substr(std::string("sqlite:").size());
    // dbmate also accepts sqlite:// — strip a leading "//" if present
    if (dbFile.rfind("//", 0) == 0) dbFile = dbFile.substr(2);
    std::string seedCmd =
        "sqlite3 \"" + dbFile + "\" < \"" + seedFile.string() + "\" 2>&1";
    if (std::system(seedCmd.c_str()) != 0) {
        LOG_ERROR << "Auto-seed: seed.sql FAILED on " << dbFile;
        return;
    }
    LOG_INFO << "Auto-seed: done (" << dbFile << ")";
}

// Resolve app root from argv[0] so all paths are absolute regardless of CWD.
static std::filesystem::path resolveAppRoot(const char *argv0) {
    auto p = std::filesystem::canonical(std::filesystem::path(argv0));
    // binary is at <root>/build/<name> or <root>/<name>
    auto parent = p.parent_path();
    // If built in a build/ subdirectory, go one level up
    if (parent.filename() == "build" || parent.filename() == "Release" ||
        parent.filename() == "Debug") {
        return parent.parent_path();
    }
    return parent;
}

static std::atomic<bool> shutdownRequested{false};

static void sigHandler(int /*sig*/) {
    shutdownRequested = true;
    drogon::app().quit();
}

int main(int argc, char *argv[]) {
    // ── 1. Resolve absolute app root ──────────────────────────────────────────
    auto appRoot = resolveAppRoot(argv[0]);
    LOG_INFO << "AppRoot: " << appRoot.string();

    // ── 2. Load .env file if present ──────────────────────────────────────────
    // Simple parser: KEY=VALUE, full-line and inline comments (` # ...`),
    // whitespace-trimmed, single/double-quoted values ('#' inside quotes is
    // kept literal). Drogon handles config.json separately.
    {
        auto trim = [](std::string &s) {
            size_t b = s.find_first_not_of(" \t\r");
            size_t e = s.find_last_not_of(" \t\r");
            s = (b == std::string::npos) ? "" : s.substr(b, e - b + 1);
        };
        auto envFile = appRoot / ".env";
        if (std::filesystem::exists(envFile)) {
            FILE *f = fopen(envFile.string().c_str(), "r");
            if (f) {
                char line[512];
                while (fgets(line, sizeof(line), f)) {
                    std::string s(line);
                    if (!s.empty() && s.back() == '\n') s.pop_back();
                    trim(s);
                    if (s.empty() || s[0] == '#') continue;
                    auto eq = s.find('=');
                    if (eq == std::string::npos) continue;
                    std::string key = s.substr(0, eq);
                    std::string val = s.substr(eq + 1);
                    trim(key);
                    trim(val);
                    if (key.empty()) continue;
                    if (val.size() >= 2 &&
                        (val.front() == '"' || val.front() == '\'') &&
                        val.back() == val.front()) {
                        // Quoted value — strip quotes, keep content verbatim
                        // (a '#' inside quotes is NOT a comment).
                        val = val.substr(1, val.size() - 2);
                    } else {
                        // Unquoted — strip inline comment: '#' at the start of
                        // the value or preceded by whitespace.
                        for (size_t i = 0; i < val.size(); ++i) {
                            if (val[i] == '#' &&
                                (i == 0 || std::isspace(
                                               static_cast<unsigned char>(
                                                   val[i - 1])))) {
                                val.erase(i);
                                break;
                            }
                        }
                        trim(val);
                    }
                    // Don't override existing env vars
                    if (!std::getenv(key.c_str())) setenv(key.c_str(), val.c_str(), 0);
                }
                fclose(f);
            }
        }
    }

    // ── 3. Validate config (fail-fast in production) ──────────────────────────
    AppConfig::instance().loadAndValidate();
    const auto &cfg = AppConfig::instance();

    // ── 4. Load Drogon config.json (APP_PORT overrides the listener) ─────────
    // Parity with NodeAdmin/RustAdmin: APP_PORT from env/.env wins over the
    // port in config.json; config.json is the fallback when APP_PORT is unset.
    // (Docker entrypoint rewrites config.json to the same APP_PORT — both
    // paths agree there.)
    auto configPath = appRoot / "config.json";
    Json::Value drogonCfg(Json::objectValue);
    bool haveConfigFile = std::filesystem::exists(configPath);
    if (haveConfigFile) {
        std::ifstream cfgIn(configPath.string());
        Json::CharReaderBuilder rb;
        std::string parseErrs;
        if (!Json::parseFromStream(rb, cfgIn, &drogonCfg, &parseErrs)) {
            LOG_FATAL << "Failed to parse " << configPath.string() << ": "
                      << parseErrs;
            return 1;
        }
    } else {
        LOG_WARN << "config.json not found at " << configPath.string()
                 << " — using defaults";
    }

    const char *appPortEnv = std::getenv("APP_PORT");
    const bool portOverridden = (appPortEnv && *appPortEnv);
    if (portOverridden) {
        if (drogonCfg["listeners"].isArray() && !drogonCfg["listeners"].empty()) {
            for (auto &listener : drogonCfg["listeners"]) {
                listener["port"] = cfg.appPort;
            }
        } else {
            Json::Value listener(Json::objectValue);
            listener["address"] = "0.0.0.0";
            listener["port"]    = cfg.appPort;
            listener["https"]   = false;
            drogonCfg["listeners"].append(listener);
        }
    }
    drogon::app().loadConfigJson(drogonCfg);

    if (portOverridden) {
        LOG_INFO << "Listening on port " << cfg.appPort << " (from APP_PORT)";
    } else if (drogonCfg["listeners"].isArray() && !drogonCfg["listeners"].empty()) {
        LOG_INFO << "Listening on port "
                 << drogonCfg["listeners"][0]["port"].asInt()
                 << " (from config.json)";
    } else {
        LOG_WARN << "No listener configured (no APP_PORT, no config.json "
                 << "listeners) — server will not accept HTTP connections";
    }

    // ── 5. Absolute paths for document root and uploads ──────────────────────
    // Views are compiled into the binary via drogon_create_views — no setViewsPath needed.
    drogon::app().setDocumentRoot((appRoot / "public").string());
    drogon::app().setUploadPath((appRoot / "storage" / "uploads").string());
    drogon::app().setStaticFilesCacheTime(cfg.isProduction() ? 604800 : 0);

    // ── 6. Graceful shutdown ──────────────────────────────────────────────────
    signal(SIGTERM, sigHandler);
    signal(SIGINT,  sigHandler);

    // ── 7. Security headers via post-handling advice ──────────────────────────
    // registerPostHandlingAdvice: (req, resp) → void; called after handler produces response.
    drogon::app().registerPostHandlingAdvice(
        [](const drogon::HttpRequestPtr &, const drogon::HttpResponsePtr &resp) {
            resp->addHeader("X-Frame-Options",        "SAMEORIGIN");
            resp->addHeader("X-Content-Type-Options", "nosniff");
            resp->addHeader("X-XSS-Protection",       "1; mode=block");
            resp->addHeader("Referrer-Policy",        "strict-origin-when-cross-origin");
            resp->addHeader("Permissions-Policy",     "camera=(), microphone=(), geolocation=()");
            if (AppConfig::instance().isProduction()) {
                resp->addHeader("Strict-Transport-Security",
                                "max-age=31536000; includeSubDomains");
            }
        });

    // ── 8. Gzip ───────────────────────────────────────────────────────────────
    drogon::app().enableGzip(true);

    // ── 9. Exception handler — map AppError → HTTP; others → 500 ─────────────
    drogon::app().setExceptionHandler(
        [](const std::exception &e,
           const drogon::HttpRequestPtr  &req,
           std::function<void(const drogon::HttpResponsePtr &)> &&callback) {
            const bool isApi = (req->path().rfind("/api/", 0) == 0);
            int code = 500;
            std::string msg = "Internal Server Error";

            if (auto *ae = dynamic_cast<const AppError *>(&e)) {
                code = ae->statusCode;
                // In production: generalize 500; expose 4xx detail
                msg = (code >= 500 && AppConfig::instance().isProduction())
                      ? "Internal Server Error" : ae->what();
            }

            if (isApi) {
                Json::Value body;
                body["success"] = false;
                body["message"] = msg;
                auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
                resp->setStatusCode((drogon::HttpStatusCode)code);
                callback(resp);
            } else {
                // Web: store flash + redirect back, or show error page
                LOG_WARN << "Web exception path=" << req->path() << " [" << code << "] " << e.what();
                req->session()->insert("flash_key",     std::string("error"));
                req->session()->insert("flash_message", msg);
                auto ref = req->getHeader("Referer");
                std::string redirTo = (!ref.empty()) ? ref : "/auth/login";
                callback(drogon::HttpResponse::newRedirectionResponse(redirTo));
            }
        });

    // ── 10. Dev: auto-migrate + seed ─────────────────────────────────────────
    autoMigrateAndSeed(cfg.appEnv, appRoot);

    // ── 11. Register routes ────────────────────────────────────────────────────
    // Route registrations also populate RouteRegistry for RBAC reverse-lookup.
    registerAuthRoutes();

    // Penyajian berkas storage lokal ("/storage/<key>") — publik, selalu aktif
    // (dibutuhkan halaman publik spt login/frontend). No-op saat driver remote.
    registerStorageRoutes();

#ifdef ENABLE_WEB_UI
    if (cfg.webUiEnabled()) {
        registerDashboardRoutes();
        registerComponentsRoutes();
        registerSettingRoutes();
        registerProfileRoutes();
        registerHomeRoutes();
        registerMediaRoutes();
        registerAccessRoutes();
    } else {
        // API-only: register access + auth API routes only
        registerAccessRoutes();
    }
#else
    registerAccessRoutes();
#endif

    // ── 12. CORS for API ──────────────────────────────────────────────────────
    // CORS in Drogon 1.8.7 is configured via config.json ("cors" section).
    // The addAllowedOrigin/setCORS API is not available in this version.
    // See config.json for CORS settings.
    (void)cfg.appUrl;

    // ── 13. Listen error fail-fast ─────────────────────────────────────────────
    try {
        LOG_INFO << "Starting CppAdmin (" << cfg.appMode << " mode, env=" << cfg.appEnv << ")";
        drogon::app().run();
    } catch (const std::exception &e) {
        LOG_FATAL << "Failed to start server: " << e.what();
        return 1;
    }

    LOG_INFO << "CppAdmin shut down gracefully.";
    return 0;
}
