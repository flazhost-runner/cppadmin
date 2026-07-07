# CppAdmin Architecture

## Overview

CppAdmin is a C++20/Drogon port of NodeAdmin. It uses coroutine-based async handlers, compile-time CSP templates, and a layered architecture identical in semantics to the TypeScript original but idiomatic C++.

## Request Lifecycle

```
HTTP Request
  │
  ▼
MethodOverrideFilter        POST+?_method=PUT → rewrites method
  │
  ▼
CsrfFilter                  Unsafe methods: verify session _csrf token
  │                         API paths (/ api/): skip
  ▼
AuthFilter                  Web: check session["currentUser"]
  │                         API: verify Bearer JWT (libjwt HS256)
  ▼
RbacFilter                  Reverse-lookup (method+path) → permission name
  │                         Administrator: bypass. No access: 403/redirect.
  ▼
RateLimitFilter             Redis INCR+EXPIRE sliding window (login, OTP)
  │
  ▼
Controller (web/v1 or api/v1)
  │   Parses req params, calls Service, builds response
  ▼
Service
  │   Business logic, DB via CoroMapper, throws AppError
  ▼
Drogon ORM (CoroMapper)
  │
  ▼
Database (SQLite/MySQL/PostgreSQL)
```

## Layer Rules

| Layer | What it does | What it must NOT do |
|---|---|---|
| Controller | Parse params, call service, render/respond | Query DB, business logic |
| Service | Business rules, DB via CoroMapper | Read env vars (`std::getenv`), render views |
| Filter | Cross-cutting: auth, CSRF, RBAC, rate-limit | Business logic, DB writes |
| Model | Drogon ORM row struct, column accessors | Business logic |
| View (CSP) | Render HTML via compile-time template | Call services, query DB |

## Directory Layout

```
CppAdmin/
├── main.cc                  Entry point: load config, register routes, run
├── config.json              Drogon runtime config (port, DB, session, Redis)
├── .env                     Secrets (not committed)
├── CMakeLists.txt
│
├── include/
│   ├── AppConfig.h          Singleton env/config reader (only getenv allowed here)
│   ├── AppError.h           AppError hierarchy (NotFound/Conflict/Validation/…)
│   ├── RouteRegistry.h      Named-route singleton + reverse lookup for RBAC
│   └── helpers/
│       ├── HtmlEscape.h     h(), hAttr(), sanitizeRichHtml()
│       ├── Pagination.h     PaginateMeta, makePaginateMeta()
│       ├── FlashHelper.h    Flash::set*/consume()
│       ├── UuidGen.h        newUuid()
│       ├── ViewHelper.h     renderView(), injectTheme()
│       └── CiLike.h         ci_like() for case-insensitive search
│
├── filters/                 HttpFilters (request pipeline)
│   ├── MethodOverrideFilter Method-override (POST→PUT/DELETE)
│   ├── CsrfFilter           CSRF token verify
│   ├── AuthFilter           Session/JWT authentication
│   ├── RbacFilter           Permission check
│   ├── RateLimitFilter      Redis sliding window
│   └── SecurityHeadersFilter CSP/HSTS/X-Frame headers
│
├── services/                Business logic
│   ├── I*Service.h          Pure abstract interface
│   ├── *Service.h/.cc       Concrete implementation
│   └── RbacHelper.cc        userHasAccess / userIsAdministrator
│
├── controllers/
│   ├── web/v1/              Web (HTML) controllers + route registrations
│   └── api/v1/              REST JSON controllers + route registrations
│
├── models/                  Drogon ORM model structs (hand-crafted, not generated)
│
├── views/
│   └── be/default/          Backend admin views (CSP compile-time templates)
│       ├── layouts/         Shared partials: head, sidebar, topbar, foot
│       ├── auth/            Login, register, forgot, reset
│       ├── dashboard/       Dashboard index
│       ├── access/          Users, Roles, Permissions CRUD
│       ├── profile/         Current user profile
│       ├── setting/         App settings + theme switcher
│       └── components/      UI component showcase
│
├── db/
│   ├── migrations/          dbmate SQL migration files
│   └── seeds/               seed.sql (INSERT OR IGNORE)
│
├── vendor/
│   └── bcrypt/              bcrypt via crypt_r() + OpenSSL RAND_bytes
│
├── test/                    Drogon unit tests (test_main.cc)
├── tools/
│   ├── check_conventions.py Convention checker (CI gate)
│   ├── make_module.sh       Module scaffolding generator
│   └── add_ui.sh            Upgrade API-only → Full mode
└── .github/workflows/ci.yml GitHub Actions CI
```

## RBAC Design

Permissions are stored as `(name, method, guard_name)` rows. A route is registered in `RouteRegistry` with a logical name (e.g. `"access.users.index"`). `RbacFilter` reverse-looks up `(method, path)` → `name`, then checks whether the current user's roles include a permission with that name. The Administrator role bypasses all checks.

API routes use the `"api."` prefix guard (`guard_name = "api"`); web routes use `"web"`.

## CSP Templates

Views are `.csp` files compiled to C++ at cmake time (`drogon_create_views()`). They are **not** runtime-rendered — any `.csp` change requires a rebuild.

**Mandatory**: all user-supplied content must be wrapped in `h()`:
```cpp
output->write(h(data["username"].asString()));
```

CSP files receive an `HttpViewData` dictionary populated by the controller before calling `renderView(res, viewName, data)`.

## Theme System

Nine palettes (Blue default). Theme is stored in `settings.theme`. On each web response, `injectTheme(data, themeName)` adds CSS variable values to the view data. The `<head>` layout partial emits a `<style>` block with `--primary`, `--secondary`, `--light`, `--dark` CSS variables.

## Storage

File uploads use a driver-aware adapter (`include/helpers/Storage.h`, drivers
`local | oss | s3`). The DB/HTML stores the object **key**; the URL is built at
request time via `storage::url(key)` — `local` yields `/storage/<key>` (served by
`StorageController`), `oss`/`s3` yield absolute presigned URLs. Backend is chosen
by `STORAGE_DRIVER` alone. See the **Storage & switching backends** section in
[../README.md](../README.md).

## Full vs API-only Build

`cmake -DENABLE_WEB_UI=ON/OFF` controls whether CSP views are compiled and web route registrations are linked. In API-only mode, all `/auth/*` endpoints use JSON I/O; the binary has no HTML output.

## Error Handling

Services throw `AppError` subclasses. `main.cc` registers a Drogon exception handler:
- Web requests: flash error + redirect to referrer (or `/dashboard`)
- API requests: `{"success":false,"message":"...","status":N}` JSON
