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
# Open http://localhost:8010 — login: admin@admin.com / 12345678
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
| profile.show             | GET    | /admin/v1/profile                        |
| profile.update           | PUT    | /admin/v1/profile/update                 |
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

### Postman

A Postman collection is provided at [`docs/postman/CppAdmin.postman_collection.json`](docs/postman/CppAdmin.postman_collection.json). Import it and set the `base_url` collection variable (default `http://localhost:8010`) to match your listener (`config.json` → `listeners[0].port`, default `8010`).

> Note: the collection is derived from the upstream NodeAdmin collection and still uses NodeAdmin's verbose route shape (e.g. `GET /api/v1/access/role/:id/edit`, `POST /api/v1/access/role/store`). CppAdmin exposes idiomatic REST routes instead (`GET /api/v1/access/roles/{id}`, `POST /api/v1/access/roles`, plural nouns). Adjust request paths accordingly until the collection is regenerated for this port.

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

## Storage & switching backends

Uploaded files (rich-text editor images, etc.) go through a single driver-aware
adapter — `include/helpers/Storage.h`. **The database/HTML stores only the object
_key_** (e.g. `editor/1699..._12345.webp`); the URL is built at request time via
`storage::url(key)`. Switching backend is a **`.env` change + restart** — no code
or view edits.

Select the backend with `STORAGE_DRIVER` (see `.env.example`):

| Driver  | Where files live                         | Render URL (`storage::url`)                               |
|---------|------------------------------------------|-----------------------------------------------------------|
| `local` | `storage/uploads/<key>` on disk          | Relative `/storage/<key>` — served by `StorageController` |
| `oss`   | Alibaba Cloud OSS bucket                 | Absolute **presigned** URL (HMAC-SHA1, TTL 6h)            |
| `s3`    | AWS S3 / S3-compatible (MinIO, R2, …)    | Absolute **presigned** URL (SigV4, TTL 6h)               |

```dotenv
# Local (default) — no credentials needed
STORAGE_DRIVER=local

# S3 / MinIO / R2
STORAGE_DRIVER=s3
STORAGE_ACCESS_KEY_ID=...
STORAGE_SECRET_ACCESS_KEY=...
STORAGE_BUCKET=my-bucket
STORAGE_REGION=us-east-1
STORAGE_ENDPOINT=          # set for MinIO/R2 (path-style); empty for AWS
STORAGE_SSL=true
```

Details:

- **local** — files are served at the stable public prefix **`/storage/<key>`**
  (route `GET /storage/(.*)`, no auth, path-traversal guarded), decoupled from
  the filesystem path (`storage/uploads`). Only active when `STORAGE_DRIVER=local`;
  remote drivers make that route a 404 since URLs are absolute.
- **oss/s3** — private buckets; access is via short-lived presigned URLs rebuilt
  on every render, so the bucket stays private and no local serving happens.
- **Uploads are git-ignored** (`storage/uploads/*`); the directory is kept via
  `storage/uploads/.gitkeep`. Nothing users upload is committed.

**Migration caveat.** Keys are backend-independent, but the _bytes_ are not
copied automatically. When moving `local → oss/s3` (or between buckets), sync the
existing objects first, then flip `STORAGE_DRIVER` and restart:

```bash
# S3-compatible
aws s3 sync storage/uploads/ s3://my-bucket/
# Alibaba OSS
ossutil cp -r storage/uploads/ oss://my-bucket/
```

**Local in production is ephemeral.** On containers/PaaS the `storage/uploads`
dir is wiped on every deploy/restart. For a durable local backend mount a
**persistent volume** at `storage/uploads`, otherwise use `oss`/`s3`.

> Note: `Setting` (logo/favicon/login image) and user avatar currently store the
> value the admin pastes (typically a `/storage/...` URL from the editor), not a
> bare key — so pre-existing records aren't rewritten when you switch drivers.
> New editor uploads always return a driver-correct URL.

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for how the adapter fits the layers.

## Architecture Constraints

- **DI**: all services implement `IXService`; controllers take `shared_ptr<IXService>` via constructor.
- **Error flow**: services `throw AppError`; global exception handler converts to HTTP.
- **No `getenv`** in services/controllers/filters — use `AppConfig::instance()` only.
- **No direct DB in controllers** — only via service.
- **RBAC** via `RouteRegistry` named-route lookup, not hard-coded permission strings.
- **CSP templates** — all user content escaped with `h()` from `HtmlEscape.h`.

See [AGENTS.md](AGENTS.md) for the full development guide.
