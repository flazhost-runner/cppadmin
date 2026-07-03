# AGENTS.md — CppAdmin Development Rules (C++20 / Drogon)

> **Single source of truth.** Every AI and developer MUST follow this when adding/modifying code.
> Mirrors NodeAdmin's AGENTS.md with native C++/Drogon idioms.
> `cmake --build build --target check_conventions` is the CI gate — violations = build failure.

Reference: `/home/mulyawan/Project/Admin/NodeAdmin/` (concepts/principles only — NOT raw code).
See also: `docs/ARCHITECTURE.md`, `docs/MODULE_GUIDE.md`, `docs/TESTING.md`.

---

## Request Lifecycle (WAJIB)

```
HTTP Request
  → MethodOverrideFilter (global) — POST+?_method → PUT/DELETE before routing
  → CsrfFilter (global web)      — generate token GET; validate POST/PUT/DELETE
  → AuthFilter  (per-route)      — session (web) / JWT Bearer (api) → set "currentUser"
  → RbacFilter  (per-route)      — RouteRegistry reverse-lookup → HasAccess(name, method)
  → Controller (thin)            — parse req, delegate to IXService, render/JSON
  → Service (implements IXService) — business logic, throw AppError on failure
  → Drogon ORM Mapper<T> / DbClient — data access
  ↘ exception → app().setExceptionHandler() — web: flash+redirect; api: JSON 4xx/5xx
```

## Core Principles (DO NOT violate)

### 1. Dependency Injection (SOLID-D)
- Service interface: `struct IXService { virtual ... = 0; virtual ~IXService() = default; };`
- Service impl: `struct XService : IXService { ... };`
- Controller: owns `std::shared_ptr<IXService>` injected via constructor.
- Wiring: `main.cc` (or module register function) calls `app().registerObject<IXService>(std::make_shared<XService>(...))`.
- **NEVER** `new XService()` inside a controller.

### 2. Error Handling (Clean Code)
- Service **throws** `AppError`/`NotFoundError`/`ConflictError`/`ValidationError`/`UnauthorizedError` from `include/AppError.h`.
- **NEVER** return error objects. **NEVER** `return nullptr` to indicate error — throw.
- `app().setExceptionHandler()` handles ALL uncaught exceptions.
- Controller has **no try/catch** except for specific local cleanup.

### 3. Separation of Concerns
- Controller: HTTP parsing + response only. Zero business logic.
- Service: all business logic. Zero HTTP concerns.
- Model/ORM: data persistence. Zero HTTP or business logic.
- View (CSP): presentation. Zero business logic.

### 4. Config — no `getenv` in modules
- Access env ONLY via `AppConfig::instance()` (loaded in `main.cc`).
- **NEVER** call `std::getenv()` inside `services/`, `controllers/`, `filters/` (except AppConfig.h itself).
- Checker enforces this: `getenv` in those paths = CI failure.

### 5. Views — CSP compile-time, htmlEscape mandatory
- All `.csp` files in `views/` are compiled at build time. **Changing a template = rebuild required.**
- **NEVER** output user-controlled values raw in CSP.
- **ALWAYS** wrap user content with `h(...)` (from `include/helpers/HtmlEscape.h`).
- Pattern: `<%c++ output->write(h(user.getValueOfName())); %>`
- Trusted values (route paths, DB-stored theme CSS vars) may bypass `h()`.
- Set `Content-Type: text/html` explicitly: `resp->setContentTypeCodeAndCustomString(drogon::CT_TEXT_HTML, "text/html; charset=utf-8")`.

### 6. Named Routes (RBAC route-driven)
- Every route = one `RouteRegistry::instance().add(name, method, path)` call.
- Name format: `{admin.v1|web|api.v1}.{module}.{resource}.{action}` (resource access = singular).
- Permission auto-scan: `PermissionService::syncFromRegistry()` → upserts permission rows from registry. Called lazy on Permission index page.
- `RbacFilter` reverse-looks up `(method, path)` → name → `userHasAccess(uid, name, method)`.
- **NEVER** hardcode permission names in business logic.

### 7. Method Override
- HTML forms only support GET/POST. PUT/DELETE are sent via `POST action="…?_method=PUT"`.
- `MethodOverrideFilter` (global, runs before routing) rewrites method before Drogon routing.
- Form deletes: `action="/path?_method=DELETE&_csrf=TOKEN"` (CSRF token in query — DELETE body not parsed).

### 8. Coroutines
- All handlers that query DB must use `drogon::AsyncTask` or return `drogon::Task<drogon::HttpResponsePtr>`.
- Use `co_await` consistently. **NEVER** mix callback-style (`execSqlAsync(callback...)`) with coroutine-style in the same handler.
- Mapper: `co_await mapper.findByPrimaryKey(id)` etc.

### 9. DB Schema — portable, PIN names
- `tableName()` MUST be overridden explicitly in every model class (default = lowercase class name ≠ canonical).
- `id` = `varchar(36)` UUID everywhere. Use `newUuid()` from `include/helpers/UuidGen.h`.
- Join tables (`users_roles`, `roles_permissions`) managed via raw `DbClient::execSqlAsync(...)` — Drogon ORM does not manage M2M declaratively.
- `desc` column = RESERVED WORD → quote in raw SQL: SQLite/PG = `"desc"`, MySQL = backtick.
- `permissions.name` is NON-unique (same permission name may exist for different methods).

### 10. Portability
- DB queries must work on SQLite (test), MySQL, and PostgreSQL.
- Case-insensitive search: use `ciLike()` from `include/helpers/CiLike.h` — NOT raw `LIKE`.
- **NEVER** use DB-specific syntax (MySQL backtick outside quotes, `AUTO_INCREMENT`, `SERIAL`, etc.) in migration SQL.

---

## Checklist: New Module

Before starting: read `docs/MODULE_GUIDE.md`. Run `cmake --build build --target check_conventions` after.

1. **Migration** `db/migrations/TIMESTAMP_name.sql` — portable SQL, dbmate up/down.
2. **Model regen** `drogon_ctl create model -d ./models . sqlite3:dev.db` after migration.
   PIN `tableName()` explicitly. Verify model class compiles.
3. **Interface** `services/IXService.h` — pure virtual, `virtual ~IXService() = default`.
4. **Service** `services/XService.h` + `.cc` — implements `IXService`, owns ORM Mapper, throws `AppError`.
5. **Controllers** `controllers/web/v1/XController.h` + `.cc` and/or `controllers/api/v1/XApiController.h` + `.cc`.
6. **Register routes** in module's `registerXRoutes()` function — both `ROUTE_REG(name, method, path)` and `app().registerHandler(path, handler, {method})`.
7. **Views** `views/be/default/{module}/` — CSP files. All user content through `h()`. Content-Type = text/html.
8. **Flash + validation** — use `Flash::setFieldErrors()` / `Flash::consume()`.
9. **Tests** — `tests/unit/`, `tests/integration/`, `tests/api/`, `tests/bdd/` (see `docs/TESTING.md`).
10. **Docs** — update `docs/API.md` for any API routes; note new module in `README.md`.

---

## DO NOT (will fail check_conventions or break runtime)

- ❌ `std::getenv()` inside `services/`, `controllers/`, `filters/` (except `AppConfig.h`).
- ❌ Controller accessing DB directly (no `Mapper<T>` or `DbClient` in controllers — only via service).
- ❌ Service without interface (`struct IXService`).
- ❌ Output user content raw in CSP without `h()`.
- ❌ Callback-style DB access mixed with coroutine-style in same handler.
- ❌ Hard-coded permission names in `RbacFilter` or service — use RouteRegistry.
- ❌ `tableName()` not overridden in model (default = lowercase class name ≠ canonical table).
- ❌ Raw `LIKE` in SQL (case-sensitivity differs MySQL/PG/SQLite) — use `ciLike()`.
- ❌ Module without test + docs update.
- ❌ Path relatif-CWD for document_root / views / storage — always absolute via `appRoot`.

---

## Security Checklist (per route)

- Authenticated web route: attach `AuthFilter` in `METHOD_LIST`.
- RBAC-protected route: attach `RbacFilter` AFTER `AuthFilter`.
- Rate-limited endpoint (login/register/reset OTP): attach `RateLimitFilter`.
- All mutating web forms: CSRF token validated by global `CsrfFilter`.
- Upload handlers: validate magic bytes (16 bytes), whitelist extension, re-encode images.
- Sanitize rich HTML before storing (`sanitizeRichHtml()`); render raw (trusted) in view.
- Error responses: generalize 500 messages in production via `setExceptionHandler`.

---

## Themes — 9 switchable palettes (DB-driven, zero rebuild)

Palet disimpan di `settings.theme`. Tiap render admin inject CSS vars ke `HttpViewData`:
`--primary`, `--secondary`, `--theme-light`, `--theme-dark`.

| Theme  | primary   | secondary | light     | dark      |
|--------|-----------|-----------|-----------|-----------|
| Blue *(default)* | `#3B82F6` | `#60A5FA` | `#DBEAFE` | `#1E40AF` |
| Black  | `#374151` | `#4B5563` | `#6B7280` | `#1F2937` |
| Brown  | `#A16207` | `#D97706` | `#FEF3C7` | `#78350F` |
| Green  | `#10B981` | `#34D399` | `#D1FAE5` | `#047857` |
| Grey   | `#6B7280` | `#9CA3AF` | `#E5E7EB` | `#374151` |
| Orange | `#F59E0B` | `#FBBF24` | `#FEF3C7` | `#D97706` |
| Purple | `#8B5CF6` | `#A78BFA` | `#F3E8FF` | `#6D28D9` |
| Red    | `#EF4444` | `#F87171` | `#FECACA` | `#B91C1C` |
| Yellow | `#F59E0B` | `#FCD34D` | `#FEF3C7` | `#D97706` |

---

## Named Routes Reference (canonical — match NodeAdmin exactly)

| Name | Method | Path |
|------|--------|------|
| `web.home.root` | GET | `/` |
| `web.home.index` | GET | `/home` |
| `web.auth.login` | GET | `/auth/login` |
| `web.auth.login.post` | POST | `/auth/login` |
| `web.auth.register` | GET | `/auth/register` |
| `web.auth.register.post` | POST | `/auth/register` |
| `web.auth.logout` | POST | `/auth/logout` |
| `admin.v1.auth.reset.req` | GET | `/admin/v1/auth/reset/req` |
| `admin.v1.auth.reset.request` | POST | `/admin/v1/auth/reset/request` |
| `admin.v1.auth.reset.proc` | GET | `/admin/v1/auth/reset/proc` |
| `admin.v1.auth.reset.process` | POST | `/admin/v1/auth/reset/process` |
| `admin.v1.dashboard.index` | GET | `/admin/v1/dashboard` |
| `admin.v1.components.index` | GET | `/admin/v1/components` |
| `admin.v1.setting.index` | GET | `/admin/v1/setting` |
| `admin.v1.setting.update` | PUT | `/admin/v1/setting/update` |
| `admin.v1.setting.fe_preview` | GET | `/admin/v1/setting/fe-preview/{slug}` |
| `admin.v1.profile.index` | GET | `/admin/v1/profile` |
| `admin.v1.profile.update` | PUT | `/admin/v1/profile/update` |
| `admin.v1.access.user.index` | GET | `/admin/v1/access/user` |
| `admin.v1.access.user.create` | GET | `/admin/v1/access/user/create` |
| `admin.v1.access.user.store` | POST | `/admin/v1/access/user/store` |
| `admin.v1.access.user.edit` | GET | `/admin/v1/access/user/{id}/edit` |
| `admin.v1.access.user.update` | PUT | `/admin/v1/access/user/{id}/update` |
| `admin.v1.access.user.delete` | DELETE | `/admin/v1/access/user/{id}/delete` |
| `admin.v1.access.user.delete_selected` | POST | `/admin/v1/access/user/delete_selected` |
| `admin.v1.access.role.index` | GET | `/admin/v1/access/role` |
| `admin.v1.access.role.create` | GET | `/admin/v1/access/role/create` |
| `admin.v1.access.role.store` | POST | `/admin/v1/access/role/store` |
| `admin.v1.access.role.edit` | GET | `/admin/v1/access/role/{id}/edit` |
| `admin.v1.access.role.update` | PUT | `/admin/v1/access/role/{id}/update` |
| `admin.v1.access.role.delete` | DELETE | `/admin/v1/access/role/{id}/delete` |
| `admin.v1.access.role.delete_selected` | POST | `/admin/v1/access/role/delete_selected` |
| `admin.v1.access.role.permission` | GET | `/admin/v1/access/role/{id}/permission` |
| `admin.v1.access.role.permission.assign` | GET | `/admin/v1/access/role/{id}/permission/{pid}/assign` |
| `admin.v1.access.role.permission.assign_selected` | POST | `/admin/v1/access/role/{id}/permission/assign_selected` |
| `admin.v1.access.role.permission.unassign` | GET | `/admin/v1/access/role/{id}/permission/{pid}/unassign` |
| `admin.v1.access.role.permission.unassign_selected` | POST | `/admin/v1/access/role/{id}/permission/unassign_selected` |
| `admin.v1.access.permission.index` | GET | `/admin/v1/access/permission` |
| `admin.v1.access.permission.create` | GET | `/admin/v1/access/permission/create` |
| `admin.v1.access.permission.store` | POST | `/admin/v1/access/permission/store` |
| `admin.v1.access.permission.edit` | GET | `/admin/v1/access/permission/{id}/edit` |
| `admin.v1.access.permission.update` | PUT | `/admin/v1/access/permission/{id}/update` |
| `admin.v1.access.permission.delete` | DELETE | `/admin/v1/access/permission/{id}/delete` |
| `admin.v1.access.permission.delete_selected` | POST | `/admin/v1/access/permission/delete_selected` |
| *(API mirrors — prefix `api.v1.*`)* | same methods | `/api/v1/...` |

---

## Definition of Done (module/feature)

- [ ] `cmake --build build --target check_conventions` → 0 violations.
- [ ] `cmake --build build` → 0 errors, 0 warnings.
- [ ] `ctest --test-dir build` → all tests green.
- [ ] Security checklist fulfilled (auth + RBAC + CSRF + rate-limit where applicable).
- [ ] All views: content-type text/html, user content via `h()`.
- [ ] Named routes registered in RouteRegistry.
- [ ] `README.md` + `docs/API.md` updated.

## Key Commands

```bash
# Build
mkdir -p build && cd build && cmake .. -DENABLE_WEB_UI=ON && make -j$(nproc)

# Build API-only mode
mkdir -p build-api && cd build-api && cmake .. -DENABLE_WEB_UI=OFF && make -j$(nproc)

# Test
ctest --test-dir build -V

# Convention check
cmake --build build --target check_conventions

# DB migrations (requires dbmate)
dbmate up          # apply pending migrations
dbmate down        # rollback last migration
dbmate new name    # create new migration file

# Regenerate ORM models after migration
drogon_ctl create model -d ./models . sqlite3:dev.db

# Generate new module skeleton
./tools/make_module.sh MyModule mymodule

# Upgrade API-only → Full
./tools/add_ui.sh
```
