#!/usr/bin/env bash
# make_module.sh — CppAdmin module generator
# Usage: ./tools/make_module.sh <ModuleName> <module_slug>
# Example: ./tools/make_module.sh Product product
#
# Generates:
#   services/I{Name}Service.h
#   services/{Name}Service.h
#   services/{Name}Service.cc
#   controllers/web/v1/{Name}Controller.h
#   controllers/web/v1/{Name}Controller.cc
#   controllers/api/v1/{Name}ApiController.h
#   controllers/api/v1/{Name}ApiController.cc
#   controllers/web/v1/{Name}Routes.cc
#   views/be/default/{slug}/index.csp
#   views/be/default/{slug}/create.csp
#   views/be/default/{slug}/edit.csp
#   tests/integration/{slug}_service_test.cc
#   tests/api/{slug}_api_test.cc
#   db/migrations/$(date +%Y%m%d%H%M%S)_create_{slug}_table.sql

set -euo pipefail

if [[ $# -lt 2 ]]; then
    echo "Usage: $0 <ModuleName> <module_slug>"
    echo "  ModuleName : PascalCase  (e.g. Product)"
    echo "  module_slug: snake_case  (e.g. product)"
    exit 1
fi

NAME="$1"    # PascalCase, e.g. Product
SLUG="$2"    # snake_case, e.g. product
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TS="$(date +%Y%m%d%H%M%S)"

echo "Generating module: $NAME ($SLUG)"
echo "Project root: $ROOT"

mkdir -p "$ROOT/services"
mkdir -p "$ROOT/controllers/web/v1"
mkdir -p "$ROOT/controllers/api/v1"
mkdir -p "$ROOT/views/be/default/$SLUG"
mkdir -p "$ROOT/tests/integration"
mkdir -p "$ROOT/tests/api"
mkdir -p "$ROOT/db/migrations"

# ── Interface ─────────────────────────────────────────────────────────────────
cat > "$ROOT/services/I${NAME}Service.h" << TMPL
#pragma once
#include <string>
#include <vector>
#include "include/helpers/Pagination.h"

struct ${NAME}ListResult {
    std::vector<std::string> ids; // placeholder until model exists
    PaginateMeta meta;
};

class I${NAME}Service {
public:
    virtual ~I${NAME}Service() = default;
    virtual ${NAME}ListResult index(const std::string &q_name, const std::string &q_status,
                                    int page, int pageSize) = 0;
    virtual void store(const std::string &name, const std::string &status) = 0;
    virtual void update(const std::string &id, const std::string &name, const std::string &status) = 0;
    virtual void remove(const std::string &id) = 0;
    virtual void deleteSelected(const std::vector<std::string> &ids) = 0;
};
TMPL

# ── Service header ─────────────────────────────────────────────────────────────
cat > "$ROOT/services/${NAME}Service.h" << TMPL
#pragma once
#include "I${NAME}Service.h"

class ${NAME}Service : public I${NAME}Service {
public:
    ${NAME}ListResult index(const std::string &q_name, const std::string &q_status,
                            int page, int pageSize) override;
    void store(const std::string &name, const std::string &status) override;
    void update(const std::string &id, const std::string &name, const std::string &status) override;
    void remove(const std::string &id) override;
    void deleteSelected(const std::vector<std::string> &ids) override;
};
TMPL

# ── Service impl ───────────────────────────────────────────────────────────────
cat > "$ROOT/services/${NAME}Service.cc" << TMPL
#include "${NAME}Service.h"
#include "include/AppError.h"
#include "include/helpers/CiLike.h"
#include "include/helpers/UuidGen.h"
#include <drogon/drogon.h>

// TODO: Include the generated model header once drogon_ctl create model is run.
// #include "${NAME}.h" (in models/)

${NAME}ListResult ${NAME}Service::index(const std::string &q_name, const std::string &q_status,
                                        int page, int pageSize) {
    // TODO: implement with Drogon ORM Mapper<${NAME}> and ciLike()
    return {};
}

void ${NAME}Service::store(const std::string &name, const std::string &status) {
    // TODO: validate + insert
    throw AppError("Not implemented", 501);
}

void ${NAME}Service::update(const std::string &id, const std::string &name, const std::string &status) {
    throw AppError("Not implemented", 501);
}

void ${NAME}Service::remove(const std::string &id) {
    throw AppError("Not implemented", 501);
}

void ${NAME}Service::deleteSelected(const std::vector<std::string> &ids) {
    throw AppError("Not implemented", 501);
}
TMPL

# ── Web controller header ──────────────────────────────────────────────────────
cat > "$ROOT/controllers/web/v1/${NAME}Controller.h" << TMPL
#pragma once
#include <drogon/HttpController.h>
#include <memory>
#include "services/I${NAME}Service.h"

class ${NAME}Controller : public drogon::HttpController<${NAME}Controller> {
public:
    explicit ${NAME}Controller(std::shared_ptr<I${NAME}Service> svc)
        : svc_(std::move(svc)) {}

    METHOD_LIST_BEGIN
    ADD_METHOD_TO(${NAME}Controller::index,  "/admin/v1/${SLUG}",             drogon::Get);
    ADD_METHOD_TO(${NAME}Controller::create, "/admin/v1/${SLUG}/create",      drogon::Get);
    ADD_METHOD_TO(${NAME}Controller::store,  "/admin/v1/${SLUG}/store",       drogon::Post);
    ADD_METHOD_TO(${NAME}Controller::edit,   "/admin/v1/${SLUG}/{id}/edit",   drogon::Get);
    ADD_METHOD_TO(${NAME}Controller::update, "/admin/v1/${SLUG}/{id}/update", drogon::Put);
    ADD_METHOD_TO(${NAME}Controller::remove, "/admin/v1/${SLUG}/{id}/delete", drogon::Delete);
    ADD_METHOD_TO(${NAME}Controller::deleteSelected, "/admin/v1/${SLUG}/delete_selected", drogon::Post);
    METHOD_LIST_END

    drogon::AsyncTask index(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb);
    drogon::AsyncTask create(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb);
    drogon::AsyncTask store(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb);
    drogon::AsyncTask edit(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb, const std::string &id);
    drogon::AsyncTask update(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb, const std::string &id);
    drogon::AsyncTask remove(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb, const std::string &id);
    drogon::AsyncTask deleteSelected(drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb);

private:
    std::shared_ptr<I${NAME}Service> svc_;
};
TMPL

# ── Web controller impl ────────────────────────────────────────────────────────
cat > "$ROOT/controllers/web/v1/${NAME}Controller.cc" << TMPL
#include "${NAME}Controller.h"
#include "include/helpers/FlashHelper.h"
#include "include/helpers/ViewHelper.h"
#include <drogon/HttpResponse.h>

drogon::AsyncTask ${NAME}Controller::index(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb) {
    int page     = std::stoi(req->getParameter("page").empty() ? "1" : req->getParameter("page"));
    int pageSize = std::stoi(req->getParameter("q_page_size").empty() ? "10" : req->getParameter("q_page_size"));
    auto result = svc_->index(req->getParameter("q_name"), req->getParameter("q_status"), page, pageSize);
    drogon::HttpViewData data;
    Flash::consume(req, data);
    data.insert("meta",   result.meta);
    data.insert("page",   page);
    data.insert("filter", req->getParameters());
    cb(renderView("be/default/${SLUG}/index", data));
    co_return;
}

drogon::AsyncTask ${NAME}Controller::create(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb) {
    drogon::HttpViewData data;
    Flash::consume(req, data);
    cb(renderView("be/default/${SLUG}/create", data));
    co_return;
}

drogon::AsyncTask ${NAME}Controller::store(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb) {
    svc_->store(req->getParameter("name"), req->getParameter("status"));
    Flash::setSuccess(req, "Store ${NAME} Success.");
    cb(drogon::HttpResponse::newRedirectionResponse("/admin/v1/${SLUG}"));
    co_return;
}

drogon::AsyncTask ${NAME}Controller::edit(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb, const std::string &id) {
    drogon::HttpViewData data;
    Flash::consume(req, data);
    data.insert("id", id);
    cb(renderView("be/default/${SLUG}/edit", data));
    co_return;
}

drogon::AsyncTask ${NAME}Controller::update(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb, const std::string &id) {
    svc_->update(id, req->getParameter("name"), req->getParameter("status"));
    Flash::setSuccess(req, "Update ${NAME} Success.");
    cb(drogon::HttpResponse::newRedirectionResponse("/admin/v1/${SLUG}"));
    co_return;
}

drogon::AsyncTask ${NAME}Controller::remove(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb, const std::string &id) {
    svc_->remove(id);
    Flash::setSuccess(req, "Delete ${NAME} Success.");
    cb(drogon::HttpResponse::newRedirectionResponse("/admin/v1/${SLUG}"));
    co_return;
}

drogon::AsyncTask ${NAME}Controller::deleteSelected(
    drogon::HttpRequestPtr req, std::function<void(drogon::HttpResponsePtr)> cb) {
    // "selected[]" param — Drogon provides repeated params as vector
    auto params = req->getParameters();
    std::vector<std::string> ids;
    auto range = params.equal_range("selected[]");
    for (auto it = range.first; it != range.second; ++it) ids.push_back(it->second);
    svc_->deleteSelected(ids);
    Flash::setSuccess(req, "Delete Selected ${NAME} Success.");
    cb(drogon::HttpResponse::newRedirectionResponse("/admin/v1/${SLUG}"));
    co_return;
}
TMPL

# ── Routes ────────────────────────────────────────────────────────────────────
cat > "$ROOT/controllers/web/v1/${NAME}Routes.cc" << TMPL
#include "include/RouteRegistry.h"

void register${NAME}Routes() {
    ROUTE_REG("admin.v1.${SLUG}.index",          "GET",    "/admin/v1/${SLUG}");
    ROUTE_REG("admin.v1.${SLUG}.create",         "GET",    "/admin/v1/${SLUG}/create");
    ROUTE_REG("admin.v1.${SLUG}.store",          "POST",   "/admin/v1/${SLUG}/store");
    ROUTE_REG("admin.v1.${SLUG}.edit",           "GET",    "/admin/v1/${SLUG}/{id}/edit");
    ROUTE_REG("admin.v1.${SLUG}.update",         "PUT",    "/admin/v1/${SLUG}/{id}/update");
    ROUTE_REG("admin.v1.${SLUG}.delete",         "DELETE", "/admin/v1/${SLUG}/{id}/delete");
    ROUTE_REG("admin.v1.${SLUG}.delete_selected","POST",   "/admin/v1/${SLUG}/delete_selected");
    // HttpController<${NAME}Controller> auto-registers handlers
}
TMPL

# ── CSP views ─────────────────────────────────────────────────────────────────
cat > "$ROOT/views/be/default/${SLUG}/index.csp" << 'TMPL'
<%c++ #include "include/helpers/HtmlEscape.h" %>
<%c++ #include "include/helpers/Pagination.h" %>
<%c++ auto title = std::string("MODULE_NAME List"); %>
<%inc be/default/layouts/head.csp %>
<%inc be/default/layouts/sidebar.csp %>
<%inc be/default/layouts/topbar.csp %>
<div class="md:ml-64 pt-16 min-h-screen p-4">
<div class="tw-card p-0 overflow-hidden">
  <div class="px-6 py-4 border-b flex items-center justify-between">
    <h2 style="color:var(--primary)">MODULE_NAME List</h2>
    <div class="btn-group btn-sm">
      <a class="btn btn-success btn-sm" href="/admin/v1/MODULE_SLUG/create"><i class="fas fa-fw fa-plus"></i> Add Data</a>
      <button class="btn btn-danger btn-sm" form="selection" formaction="/admin/v1/MODULE_SLUG/delete_selected" data-confirm="Delete selected items?"><i class="fas fa-fw fa-times"></i> Delete Selected</button>
    </div>
  </div>
  <div class="p-4 overflow-x-auto">
    <form id="searchform" method="GET" action="/admin/v1/MODULE_SLUG">
    <table class="table table-bordered table-hover align-middle">
      <thead>
        <tr>
          <th></th>
          <th><select name="q_page_size" class="form-control form-control-sm" onchange="this.form.submit()">
            <option>10</option><option>20</option><option>50</option><option>100</option>
          </select></th>
          <th><input name="q_name" class="form-control form-control-sm" placeholder="Name..."></th>
          <th><select name="q_status" class="form-control form-control-sm">
            <option value="">All</option><option>Active</option><option>Inactive</option>
          </select></th>
          <th><div class="btn-group">
            <button class="btn btn-sm btn-success" type="submit"><i class="fas fa-search"></i></button>
            <a class="btn btn-sm btn-danger" href="/admin/v1/MODULE_SLUG"><i class="fas fa-times"></i></a>
          </div></th>
        </tr>
        <tr>
          <th><input type="checkbox" id="checkall"></th>
          <th>No</th><th>Name</th><th>Status</th><th>Action</th>
        </tr>
      </thead>
      <tbody>
        <%c++
        // TODO: iterate rows here when model is available
        %>
      </tbody>
    </table>
    </form>
    <form id="selection" method="POST"></form>
  </div>
</div>
</div>
<%inc be/default/layouts/foot.csp %>
TMPL

sed -i "s/MODULE_NAME/$NAME/g; s/MODULE_SLUG/$SLUG/g" "$ROOT/views/be/default/${SLUG}/index.csp"

cat > "$ROOT/views/be/default/${SLUG}/create.csp" << 'TMPL'
<%c++ #include "include/helpers/HtmlEscape.h" %>
<%c++ auto title = std::string("Create MODULE_NAME"); %>
<%inc be/default/layouts/head.csp %>
<%inc be/default/layouts/sidebar.csp %>
<%inc be/default/layouts/topbar.csp %>
<div class="md:ml-64 pt-16 min-h-screen p-4">
<div class="tw-card">
  <h2 class="mb-4" style="color:var(--primary)">Create MODULE_NAME</h2>
  <form method="POST" action="/admin/v1/MODULE_SLUG/store">
    <%c++ auto csrf = req->session()->getOptional<std::string>("_csrf"); %>
    <%c++ if(csrf) { output->write(R"(<input type="hidden" name="_csrf" value=")"); output->write(h(*csrf)); output->write(R"(">)"); } %>
    <div class="mb-3">
      <label class="form-label">Name</label>
      <input name="name" class="form-control" required>
    </div>
    <div class="mb-3">
      <label class="form-label">Status</label>
      <select name="status" class="form-control">
        <option value="Active">Active</option>
        <option value="Inactive">Inactive</option>
      </select>
    </div>
    <div class="btn-group">
      <button type="submit" class="btn btn-primary">Save</button>
      <a href="/admin/v1/MODULE_SLUG" class="btn btn-secondary">Cancel</a>
    </div>
  </form>
</div>
</div>
<%inc be/default/layouts/foot.csp %>
TMPL

sed -i "s/MODULE_NAME/$NAME/g; s/MODULE_SLUG/$SLUG/g" "$ROOT/views/be/default/${SLUG}/create.csp"

cp "$ROOT/views/be/default/${SLUG}/create.csp" "$ROOT/views/be/default/${SLUG}/edit.csp"
sed -i "s/Create MODULE_NAME/Edit MODULE_NAME/g" "$ROOT/views/be/default/${SLUG}/edit.csp"

# ── Migration ─────────────────────────────────────────────────────────────────
MIGFILE="$ROOT/db/migrations/${TS}_create_${SLUG}_table.sql"
cat > "$MIGFILE" << TMPL
-- migrate:up
CREATE TABLE IF NOT EXISTS ${SLUG}s (
    id          VARCHAR(36)  NOT NULL PRIMARY KEY,
    name        VARCHAR(255) NOT NULL,
    status      VARCHAR(20)  NOT NULL DEFAULT 'Active',
    created_by  VARCHAR(36),
    updated_by  VARCHAR(36),
    created_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    updated_at  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
);
CREATE INDEX IF NOT EXISTS ${SLUG}s__name ON ${SLUG}s(name);

-- migrate:down
DROP TABLE IF EXISTS ${SLUG}s;
TMPL

# ── Integration test stub ─────────────────────────────────────────────────────
cat > "$ROOT/tests/integration/${SLUG}_service_test.cc" << TMPL
#define DROGON_TEST_MAIN
#include <drogon/drogon_test.h>
#include "services/${NAME}Service.h"

DROGON_TEST(${NAME}ServiceStub) {
    // TODO: add integration tests once model is generated
    CHECK(true);
}

int main(int argc, char **argv) { return drogon::test::run(argc, argv); }
TMPL

echo ""
echo "✓ Module $NAME ($SLUG) generated."
echo ""
echo "Next steps:"
echo "  1. Run dbmate migration:  dbmate up"
echo "  2. Regenerate models:     drogon_ctl create model -d ./models . sqlite3:dev.db"
echo "  3. Add ${NAME}Service wiring in main.cc registerXRoutes() section."
echo "  4. Implement ${NAME}Service.cc (replace TODO placeholders)."
echo "  5. Implement views under views/be/default/${SLUG}/."
echo "  6. Add tests in tests/integration/ and tests/api/."
echo "  7. cmake --build build"
echo "  8. cmake --build build --target check_conventions"
