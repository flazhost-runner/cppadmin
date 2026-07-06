# CppAdmin

A C++20/Drogon 1.8.7 port of [NodeAdmin](../NodeAdmin/) — a full-stack admin panel with RBAC, JWT API, and server-side-rendered web UI.

`/` menyajikan template frontend (landing) sesuai `settings.fe_template` — katalog 640 landing [opentailwind](https://github.com/lindoai/opentailwind) diunduh on-demand & di-cache di `storage/fe/templates/`; slug khusus `default` merender landing v6 lokal (`views/fe/deflt`). Switcher lengkap (preview, pencarian, kategori, paginasi) ada di halaman Setting.

## Stack

| Layer        | Technology                           |
|--------------|--------------------------------------|
| Language     | C++20                                |
| Framework    | Drogon 1.8.7 (coroutines, ORM, CSP) |
| Database     | SQLite (dev/test) — portable schema  |
| Auth         | bcrypt passwords, JWT Bearer (API), session cookie (web) |
| Migrations   | dbmate                               |
| Tests        | Drogon test framework (ctest)        |

## Quick Start

```bash
# 1. Install dependencies (Ubuntu 24.04)
sudo apt-get install -y cmake g++ libdrogon-dev libsqlite3-dev libssl-dev

# 2. Create .env
cp .env.example .env
# Edit SESSION_SECRET and JWT_SECRET (min 32 chars each)

# 3. Build
mkdir -p build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release && make -j$(nproc) && cd ..

# 4. Run migrations + seed
DATABASE_URL=sqlite:./dev.db /tmp/dbmate up
sqlite3 dev.db < db/seeds/seed.sql

# 5. Run
./build/CppAdmin
# Open http://localhost:3000 — login: admin@admin.com / 12345678
```

## Project Structure

```
CppAdmin/
├── controllers/
│   ├── api/v1/          # JSON API controllers (HttpController<T> + METHOD_LIST)
│   └── web/v1/          # Web controllers (plain class + lambda routes)
├── services/            # Business logic (IXService interface + XService impl)
├── models/              # Hand-crafted Drogon ORM models (CoroMapper-compatible)
├── views/be/admin/      # CSP templates compiled at build time
├── filters/             # AuthFilter, CsrfFilter, RbacFilter, RateLimitFilter, MethodOverrideFilter
├── include/
│   ├── AppError.h       # AppError / NotFoundError / ConflictError / ValidationError
│   ├── AppConfig.h      # Singleton env config (reads .env via dotenv)
│   ├── RouteRegistry.h  # Named route → RBAC reverse lookup
│   └── helpers/         # HtmlEscape, Pagination, CiLike, JwtHelper, UuidGen, Flash
├── db/
│   ├── migrations/      # dbmate SQL migrations (migrate:up / migrate:down)
│   └── seeds/           # Idempotent seed data (admin user, default roles)
├── test/                # Unit + bcrypt/JWT tests (Drogon test framework)
├── tools/
│   ├── check_conventions.py   # CI gate — enforces DI, no getenv in modules, etc.
│   ├── make_module.sh         # Scaffold a new module skeleton
│   └── add_ui.sh              # Upgrade api-only → full (web+api)
└── vendor/bcrypt/       # Thin bcrypt wrapper around glibc crypt_r()
```

## Web Routes

| Name                     | Method | Path                                     |
|--------------------------|--------|------------------------------------------|
| web.auth.login           | GET    | /auth/login                              |
| web.auth.login.post      | POST   | /auth/login                              |
| web.auth.signup          | GET    | /auth/signup                             |
| web.auth.logout          | POST   | /auth/logout                             |
| dashboard.index          | GET    | /admin/v1/dashboard                      |
| access.users.*           | CRUD   | /admin/v1/access/users[/{id}[/edit]]     |
| access.roles.*           | CRUD   | /admin/v1/access/roles[/{id}[/edit]]     |
| access.permissions.*     | CRUD   | /admin/v1/access/permissions[/{id}[/edit]] |
| profile.*                | —      | /admin/v1/profile[/edit][/password]      |
| setting.edit             | GET    | /admin/v1/setting                        |
| setting.update           | PUT    | /admin/v1/setting                        |
| setting.fe_preview       | GET    | /admin/v1/setting/fe-preview/{slug}      |
| web.home.root            | GET    | /                                        |
| components.index         | GET    | /admin/v1/components                     |

## API Routes

Base: `/api/v1` — all responses `application/json`.

| Name                          | Method | Path                                    |
|-------------------------------|--------|-----------------------------------------|
| api.v1.auth.login             | POST   | /api/v1/auth/login                      |
| api.v1.auth.register          | POST   | /api/v1/auth/register                   |
| api.v1.access.users.*         | CRUD   | /api/v1/access/users[/{id}]             |
| api.v1.access.roles.*         | CRUD   | /api/v1/access/roles[/{id}]             |
| api.v1.access.permissions.*   | CRUD   | /api/v1/access/permissions[/{id}]       |

See [docs/API.md](docs/API.md) for full request/response shapes.

## Development

```bash
# Convention check (CI gate)
cmake --build build --target check_conventions

# Tests
cmake --build build -j$(nproc) && build/test/CppAdmin_test

# Add a new module
./tools/make_module.sh Widget widget

# Add web UI to an api-only module
./tools/add_ui.sh

# DB migrations
DATABASE_URL=sqlite:./dev.db /tmp/dbmate up        # apply
DATABASE_URL=sqlite:./dev.db /tmp/dbmate rollback  # undo last
DATABASE_URL=sqlite:./dev.db /tmp/dbmate new name  # new migration file
```

## Architecture Constraints

- **DI**: all services implement `IXService`; controllers take `shared_ptr<IXService>` via constructor.
- **Error flow**: services `throw AppError`; global exception handler converts to HTTP.
- **No `getenv`** in services/controllers/filters — use `AppConfig::instance()` only.
- **No direct DB in controllers** — only via service.
- **RBAC** via `RouteRegistry` named-route lookup, not hard-coded permission strings.
- **CSP templates** — all user content escaped with `h()` from `HtmlEscape.h`.

See [AGENTS.md](AGENTS.md) for the full development guide.
