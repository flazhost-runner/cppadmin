# Module Guide

How to add a new feature module to CppAdmin. Use `tools/make_module.sh` for scaffolding.

## Quick Start

```bash
./tools/make_module.sh Product product
# Creates: IProductService.h, ProductService.h/.cc, ProductController.h/.cc,
#          ProductRoutes.cc, API controller/routes, 3 CSP views, test stub, migration SQL
```

Then follow the checklist below.

## Artefact Matrix

Decide which artefacts your module needs before writing code.

| Need | Artefacts required |
|---|---|
| Web CRUD (HTML) | I*Service + *Service + *Controller (web) + *Routes (web) + views |
| API CRUD (JSON) | I*Service + *Service + *Controller (api) + *Routes (api) |
| Both | All of the above, sharing the same service |
| Read-only list | Index view/endpoint only — no create/edit/delete |
| Writes need RBAC | Add permission rows to migration + seed |

## Step-by-Step

### 1. Migration

Create `db/migrations/YYYYMMDDHHMMSS_create_<module>.sql`:

```sql
-- migrate:up
CREATE TABLE products (
    id          varchar(36)  NOT NULL PRIMARY KEY,
    name        varchar(255) NOT NULL,
    status      varchar(20)  NOT NULL DEFAULT 'Active',
    created_by  varchar(255),
    updated_by  varchar(255),
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP
);
-- migrate:down
DROP TABLE products;
```

Run migration: `dbmate up`

### 2. Model

After migration, regenerate:
```bash
drogon_ctl create model . -f sqlite3:./dev.db
```

Or hand-craft `models/Products.h/.cc` following the pattern in `models/Users.h`.

Always set `tableName` explicitly:
```cpp
static const constexpr char *tableName = "products";
```

### 3. Service Interface

`services/IProductService.h`:
```cpp
#pragma once
#include "../models/Products.h"
#include <drogon/drogon.h>
#include <string>
#include <vector>

struct IProductService {
    virtual ~IProductService() = default;
    virtual drogon::Task<std::vector<drogon_model::cppadmin::Products>>
        list(int page, int pageSize, const std::string &q) = 0;
    virtual drogon::Task<drogon_model::cppadmin::Products>
        findById(const std::string &id) = 0;
    virtual drogon::Task<drogon_model::cppadmin::Products>
        create(/* input struct */) = 0;
    virtual drogon::Task<drogon_model::cppadmin::Products>
        update(const std::string &id, /* input struct */) = 0;
    virtual drogon::Task<void> remove(const std::string &id) = 0;
};
```

### 4. Service Implementation

`services/ProductService.h`:
```cpp
#pragma once
#include "IProductService.h"
#include <drogon/orm/CoroMapper.h>

struct ProductService : IProductService {
    drogon::Task<std::vector<drogon_model::cppadmin::Products>>
        list(int page, int pageSize, const std::string &q) override;
    // ... other overrides
};
```

`services/ProductService.cc`:
- Include `AppError.h`, `AppConfig.h`, model headers, `drogon/orm/CoroMapper.h`
- All methods `drogon::Task<...>` with `co_await CoroMapper<Products>(...)`
- Throw `NotFoundError`, `ConflictError`, `ValidationError` — never return errors

### 5. Web Controller

`controllers/web/v1/ProductController.h`:
```cpp
#pragma once
#include <drogon/HttpController.h>
#include "../../../services/IProductService.h"
#include <memory>

class ProductController {
public:
    explicit ProductController(std::shared_ptr<IProductService> svc)
        : svc_(std::move(svc)) {}

    drogon::AsyncTask index(drogon::HttpRequestPtr req,
                            std::function<void(const drogon::HttpResponsePtr &)> cb);
    drogon::AsyncTask show(drogon::HttpRequestPtr req,
                           std::function<void(const drogon::HttpResponsePtr &)> cb,
                           std::string id);
    drogon::AsyncTask create(drogon::HttpRequestPtr req,
                             std::function<void(const drogon::HttpResponsePtr &)> cb);
    drogon::AsyncTask store(drogon::HttpRequestPtr req,
                            std::function<void(const drogon::HttpResponsePtr &)> cb);
    drogon::AsyncTask edit(drogon::HttpRequestPtr req,
                           std::function<void(const drogon::HttpResponsePtr &)> cb,
                           std::string id);
    drogon::AsyncTask update(drogon::HttpRequestPtr req,
                             std::function<void(const drogon::HttpResponsePtr &)> cb,
                             std::string id);
    drogon::AsyncTask destroy(drogon::HttpRequestPtr req,
                              std::function<void(const drogon::HttpResponsePtr &)> cb,
                              std::string id);
private:
    std::shared_ptr<IProductService> svc_;
};
```

`controllers/web/v1/ProductController.cc`: Use `renderView()` and `Flash::*`.

### 6. Route Registration

`controllers/web/v1/ProductRoutes.cc`:
```cpp
#include "ProductController.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <memory>

void registerProductRoutes() {
    auto svc  = std::make_shared<ProductService>();
    auto ctrl = std::make_shared<ProductController>(svc);

    ROUTE_REG("products.index",  "GET",    "/products");
    ROUTE_REG("products.create", "GET",    "/products/create");
    ROUTE_REG("products.store",  "POST",   "/products");
    ROUTE_REG("products.show",   "GET",    "/products/{id}");
    ROUTE_REG("products.edit",   "GET",    "/products/{id}/edit");
    ROUTE_REG("products.update", "PUT",    "/products/{id}");
    ROUTE_REG("products.delete", "DELETE", "/products/{id}");

    drogon::app().registerHandler("/products",
        [ctrl](auto req, auto cb) { return ctrl->index(req, cb); },
        {drogon::Get}, {"AuthFilter", "RbacFilter"});

    drogon::app().registerHandler("/products/create",
        [ctrl](auto req, auto cb) { return ctrl->create(req, cb); },
        {drogon::Get}, {"AuthFilter", "RbacFilter"});

    drogon::app().registerHandler("/products",
        [ctrl](auto req, auto cb) { return ctrl->store(req, cb); },
        {drogon::Post}, {"AuthFilter", "CsrfFilter", "RbacFilter"});

    drogon::app().registerHandler("/products/{id}",
        [ctrl](auto req, auto cb, auto id) { return ctrl->show(req, cb, id); },
        {drogon::Get}, {"AuthFilter", "RbacFilter"});

    drogon::app().registerHandler("/products/{id}/edit",
        [ctrl](auto req, auto cb, auto id) { return ctrl->edit(req, cb, id); },
        {drogon::Get}, {"AuthFilter", "RbacFilter"});

    drogon::app().registerHandler("/products/{id}",
        [ctrl](auto req, auto cb, auto id) { return ctrl->update(req, cb, id); },
        {drogon::Put}, {"AuthFilter", "CsrfFilter", "RbacFilter"});

    drogon::app().registerHandler("/products/{id}",
        [ctrl](auto req, auto cb, auto id) { return ctrl->destroy(req, cb, id); },
        {drogon::Delete}, {"AuthFilter", "CsrfFilter", "RbacFilter"});
}
```

### 7. Register in main.cc

Add to `main.cc`:
```cpp
void registerProductRoutes();    // declaration

// In registerModuleRoutes():
#ifdef ENABLE_WEB_UI
    registerProductRoutes();
#endif
```

### 8. Views

`views/be/default/products/index.csp`:
- Always use `h()` for user data
- Use `%%inc views/be/default/layouts/head.csp %%` to include layout partials
- Access view data via `$$data["key"].asString()$$`

### 9. Permissions (seed)

Add to `db/seeds/seed.sql` or a separate migration:
```sql
INSERT OR IGNORE INTO permissions (id, name, guard_name, method, status, "desc")
VALUES
  (lower(hex(randomblob(16))), 'products.index',  'web', 'GET',    'Active', 'List products'),
  (lower(hex(randomblob(16))), 'products.create', 'web', 'GET',    'Active', 'Create product form'),
  (lower(hex(randomblob(16))), 'products.store',  'web', 'POST',   'Active', 'Store product'),
  (lower(hex(randomblob(16))), 'products.show',   'web', 'GET',    'Active', 'Show product'),
  (lower(hex(randomblob(16))), 'products.edit',   'web', 'GET',    'Active', 'Edit product form'),
  (lower(hex(randomblob(16))), 'products.update', 'web', 'PUT',    'Active', 'Update product'),
  (lower(hex(randomblob(16))), 'products.delete', 'web', 'DELETE', 'Active', 'Delete product');
```

### 10. Tests

`test/test_product.cc` — integration test exercising service layer with real SQLite:
```cpp
DROGON_TEST(ProductServiceCreate) {
    // setup, call service, assert
}
```

### 11. Convention Checker

Run before committing:
```bash
cmake --build build --target check_conventions
```

Checks:
- No `std::getenv()` in services/controllers/filters
- No direct DB access in controllers
- Every `*Service.cc` has an `I*Service.h`
- Every `*Routes.cc` calls `ROUTE_REG()`
- CSP outputs use `h()`
- Models have `tableName`

## Naming Conventions

| Item | Convention | Example |
|---|---|---|
| Service interface | `I{Name}Service` | `IProductService` |
| Service impl | `{Name}Service` | `ProductService` |
| Web controller | `{Name}Controller` | `ProductController` |
| API controller | `{Name}ApiController` | `ProductApiController` |
| Route reg fn | `register{Name}Routes()` | `registerProductRoutes()` |
| Permission name | `{module}.{action}` | `products.index` |
| View path | `{module}/{file}.csp` | `products/index.csp` |
| Migration file | `YYYYMMDDHHMMSS_create_{module}.sql` | `20240201000001_create_products.sql` |
