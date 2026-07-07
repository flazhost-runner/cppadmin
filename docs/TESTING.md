# Testing Guide

## Framework

CppAdmin uses Drogon's built-in test framework (`drogon/drogon_test.h`). Tests are in `test/` and compiled as a separate target (`cppadmin_test`).

## Running Tests

```bash
# All tests
cd build && ctest --output-on-failure -V

# Rebuild + test
cmake --build build -j$(nproc) && cd build && ctest -V

# Convention checker
cmake --build build --target check_conventions
```

## Test Structure

```
test/
├── test_main.cc          Unit tests: HtmlEscape, Pagination, RouteRegistry, CiLike
├── test_auth.cc          Auth service integration tests
├── test_access.cc        Access module (User/Role/Permission) integration tests
└── test_<module>.cc      Per-module integration tests
```

## Writing Unit Tests

```cpp
#include <drogon/drogon_test.h>

DROGON_TEST(HtmlEscapeBasic) {
    CHECK(h("<b>") == "&lt;b&gt;");
    CHECK(h("A&B") == "A&amp;B");
    CHECK(h("") == "");
}
```

## Writing Integration Tests

Integration tests hit the real SQLite database. Start Drogon's event loop before running any async operation:

```cpp
#include <drogon/drogon_test.h>
#include <drogon/drogon.h>
#include "../services/AuthService.h"

// Run once per test binary
static std::once_flag loopFlag;
static void ensureLoop() {
    std::call_once(loopFlag, [] {
        std::thread([] { drogon::app().run(); }).detach();
        // Allow event loop to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    });
}

DROGON_TEST(AuthLogin_InvalidCredentials) {
    ensureLoop();
    auto svc = std::make_shared<AuthService>();
    bool threw = false;
    try {
        auto result = drogon::sync_wait(svc->login("noone@test.com", "wrong"));
    } catch (const UnauthorizedError &) {
        threw = true;
    }
    CHECK(threw);
}
```

## Test Database

Integration tests use the same SQLite dev.db configured in `config.json`. The seed data (`db/seeds/seed.sql`) is applied on first startup if `autoMigrateAndSeed()` runs (non-production only).

For isolated test runs, point `DB_NAME` at a separate file:
```bash
DB_NAME=./test.db ./build/cppadmin_test
```

## Convention Checker as a Test Gate

The `check_conventions` cmake target runs `tools/check_conventions.py`. It exits non-zero on violations. In CI this is run as a separate step before `ctest`.

Violations that cause CI failure (exit 1):
- `std::getenv()` in services, controllers, or filters
- Direct DB access (`CoroMapper`, `getDbClient`, `execSqlAsync`) in controllers
- `*Service.cc` without matching `I*Service.h`

Warnings (printed but CI still passes):
- `*Routes.cc` missing `ROUTE_REG()`
- CSP output without `h()` wrapper
- Model header missing `tableName`

## Test Coverage Goals

| Component | Test Type | What to Test |
|---|---|---|
| `h()`, Pagination, RouteRegistry | Unit | Edge cases, boundary values |
| AuthService.login | Integration | Wrong password, inactive user, blocked user, valid login |
| AuthService.registerUser | Integration | Duplicate email, successful registration |
| AuthService.processPasswordReset | Integration | Invalid OTP, expired OTP, valid reset |
| UserService | Integration | Create, list (pagination+search), update, delete, duplicate email/code |
| RoleService | Integration | Create, assign permissions, detect duplicate |
| RbacFilter | Integration | Unauthorized path, authorized path, Administrator bypass |
| API auth endpoints | HTTP | POST /api/v1/auth/login JSON in/out |

## Manual API Testing (Postman)

Import [`docs/postman/CppAdmin.postman_collection.json`](postman/CppAdmin.postman_collection.json)
and set the `base_url` collection variable to `http://localhost:8010` (the
default listener port from `config.json`). Authenticate via `POST /api/v1/auth/login`
first, then exercise the access endpoints.

> The collection is derived from the upstream NodeAdmin collection and uses
> NodeAdmin's verbose route shape (e.g. `GET /api/v1/access/role/:id/edit`).
> CppAdmin uses idiomatic REST routes (`GET /api/v1/access/roles/{id}`, plural
> nouns); adjust request paths accordingly.
