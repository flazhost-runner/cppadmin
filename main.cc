#include <drogon/drogon.h>
#include <csignal>
#include <atomic>
#include <cstdlib>
#include <filesystem>
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

// Dev-only: auto-migrate + seed so `run` works immediately without manual setup.
static void autoMigrateAndSeed(const std::string &appEnv) {
    if (appEnv == "production") return;
    // dbmate up (non-interactive) — migrations live in db/migrations/
    // Requires dbmate in PATH; silently skips if not found.
    int rc = std::system("which dbmate >/dev/null 2>&1 && dbmate up 2>&1 || true");
    (void)rc;
    LOG_INFO << "Auto-migrate: done (appEnv=" << appEnv << ")";
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
    // Simple parser: KEY=VALUE, skip comments. Drogon handles config.json separately.
    {
        auto envFile = appRoot / ".env";
        if (std::filesystem::exists(envFile)) {
            FILE *f = fopen(envFile.string().c_str(), "r");
            if (f) {
                char line[512];
                while (fgets(line, sizeof(line), f)) {
                    std::string s(line);
                    if (!s.empty() && s.back() == '\n') s.pop_back();
                    if (s.empty() || s[0] == '#') continue;
                    auto eq = s.find('=');
                    if (eq == std::string::npos) continue;
                    std::string key = s.substr(0, eq);
                    std::string val = s.substr(eq + 1);
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

    // ── 4. Load Drogon config.json ────────────────────────────────────────────
    auto configPath = appRoot / "config.json";
    if (std::filesystem::exists(configPath)) {
        drogon::app().loadConfigFile(configPath.string());
    } else {
        LOG_WARN << "config.json not found at " << configPath.string()
                 << " — using defaults";
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
    autoMigrateAndSeed(cfg.appEnv);

    // ── 11. Register routes ────────────────────────────────────────────────────
    // Route registrations also populate RouteRegistry for RBAC reverse-lookup.
    registerAuthRoutes();

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
