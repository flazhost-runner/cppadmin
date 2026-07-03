#include "AccessApiController.h"
#include <drogon/HttpResponse.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpResponse;

drogon::HttpResponsePtr AccessApiController::jsonOk(Json::Value body,
                                                      drogon::HttpStatusCode code) {
    body["status"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

static Json::Value paginateMeta(long long total, int page, int pageSize) {
    int totalPages = pageSize > 0 ? (int)((total + pageSize - 1) / pageSize) : 1;
    Json::Value m;
    m["total"]      = (Json::Int64)total;
    m["page"]       = page;
    m["pageSize"]   = pageSize;
    m["totalPages"] = totalPages;
    return m;
}

static std::string actorFromReq(const drogon::HttpRequestPtr &req) {
    auto attrs = req->getAttributes();
    return attrs->find("currentUser") ? attrs->get<std::string>("currentUser") : "system";
}

// ── Users ─────────────────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessApiController::usersIndex(HttpRequestPtr req) {
    int page     = std::max(1, atoi(req->getParameter("page").c_str()));
    int pageSize = atoi(req->getParameter("pageSize").c_str());
    if (pageSize <= 0) pageSize = 10;
    pageSize = std::min(100, pageSize);
    std::string q = req->getParameter("q");

    auto result = co_await userSvc_->list(page, pageSize, q, "", "");

    Json::Value data(Json::arrayValue);
    for (const auto &u : result.rows)
        data.append(u.toJson());

    Json::Value body;
    body["message"] = "";
    body["data"] = data;
    body["meta"] = paginateMeta(result.total, page, pageSize);
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::usersShow(HttpRequestPtr req, std::string id) {
    auto user = co_await userSvc_->findById(id);
    Json::Value body;
    body["message"] = "";
    body["data"]    = user.toJson();
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::usersStore(HttpRequestPtr req) {
    auto j = req->getJsonObject();
    if (!j) throw ValidationError("Invalid JSON body.");

    UserCreateInput input;
    input.name     = (*j).get("name",    "").asString();
    input.code     = (*j).get("code",    "").asString();
    input.email    = (*j).get("email",   "").asString();
    input.phone    = (*j).get("phone",   "").asString();
    input.password = (*j).get("password","").asString();
    input.status   = (*j).get("status",  "Active").asString();
    input.timezone = (*j).get("timezone","UTC").asString();

    auto user = co_await userSvc_->create(input, actorFromReq(req));
    Json::Value body;
    body["message"] = "Create User Success.";
    body["data"]    = user.toJson();
    co_return jsonOk(std::move(body), drogon::k201Created);
}

drogon::Task<HttpResponsePtr> AccessApiController::usersUpdate(HttpRequestPtr req, std::string id) {
    auto j = req->getJsonObject();
    if (!j) throw ValidationError("Invalid JSON body.");

    UserUpdateInput input;
    input.name     = (*j).get("name",    "").asString();
    input.phone    = (*j).get("phone",   "").asString();
    input.status   = (*j).get("status",  "").asString();
    input.timezone = (*j).get("timezone","").asString();

    auto user = co_await userSvc_->update(id, input, actorFromReq(req));
    Json::Value body;
    body["message"] = "Update User Success.";
    body["data"]    = user.toJson();
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::usersDestroy(HttpRequestPtr req, std::string id) {
    co_await userSvc_->remove(id);
    Json::Value body;
    body["message"] = "Delete User Success.";
    body["data"]    = Json::nullValue;
    co_return jsonOk(std::move(body));
}

// ── Roles ─────────────────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessApiController::rolesIndex(HttpRequestPtr req) {
    int page     = std::max(1, atoi(req->getParameter("page").c_str()));
    int pageSize = 10;
    std::string q = req->getParameter("q");

    auto result = co_await roleSvc_->list(page, pageSize, q, "");

    Json::Value data(Json::arrayValue);
    for (const auto &r : result.rows)
        data.append(r.toJson());

    Json::Value body;
    body["message"] = "";
    body["data"]    = data;
    body["meta"]    = paginateMeta(result.total, page, pageSize);
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::rolesShow(HttpRequestPtr req, std::string id) {
    auto role  = co_await roleSvc_->findById(id);
    auto perms = co_await roleSvc_->permissionsOf(id);

    Json::Value rJson = role.toJson();
    Json::Value pArr(Json::arrayValue);
    for (const auto &p : perms) pArr.append(p.toJson());
    rJson["permissions"] = pArr;

    Json::Value body;
    body["message"] = "";
    body["data"]    = rJson;
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::rolesStore(HttpRequestPtr req) {
    auto j = req->getJsonObject();
    if (!j) throw ValidationError("Invalid JSON body.");

    RoleCreateInput input;
    input.name      = (*j).get("name",       "").asString();
    input.guardName = (*j).get("guard_name", "web").asString();
    input.status    = (*j).get("status",     "Active").asString();
    input.desc      = (*j).get("desc",       "").asString();

    auto role = co_await roleSvc_->create(input, actorFromReq(req));
    Json::Value body;
    body["message"] = "Create Role Success.";
    body["data"]    = role.toJson();
    co_return jsonOk(std::move(body), drogon::k201Created);
}

drogon::Task<HttpResponsePtr> AccessApiController::rolesUpdate(HttpRequestPtr req, std::string id) {
    auto j = req->getJsonObject();
    if (!j) throw ValidationError("Invalid JSON body.");

    RoleUpdateInput input;
    input.name      = (*j).get("name",       "").asString();
    input.guardName = (*j).get("guard_name", "").asString();
    input.status    = (*j).get("status",     "").asString();
    input.desc      = (*j).get("desc",       "").asString();

    auto role = co_await roleSvc_->update(id, input, actorFromReq(req));
    Json::Value body;
    body["message"] = "Update Role Success.";
    body["data"]    = role.toJson();
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::rolesDestroy(HttpRequestPtr req, std::string id) {
    co_await roleSvc_->remove(id);
    Json::Value body;
    body["message"] = "Delete Role Success.";
    body["data"]    = Json::nullValue;
    co_return jsonOk(std::move(body));
}

// ── Permissions ───────────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessApiController::permsIndex(HttpRequestPtr req) {
    int page     = std::max(1, atoi(req->getParameter("page").c_str()));
    int pageSize = 20;
    std::string q         = req->getParameter("q");
    std::string guardName = req->getParameter("guard_name");

    auto result = co_await permSvc_->list(page, pageSize, q, guardName, "", "");

    Json::Value data(Json::arrayValue);
    for (const auto &p : result.rows)
        data.append(p.toJson());

    Json::Value body;
    body["message"] = "";
    body["data"]    = data;
    body["meta"]    = paginateMeta(result.total, page, pageSize);
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::permsShow(HttpRequestPtr req, std::string id) {
    auto perm = co_await permSvc_->findById(id);
    Json::Value body;
    body["message"] = "";
    body["data"]    = perm.toJson();
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::permsStore(HttpRequestPtr req) {
    auto j = req->getJsonObject();
    if (!j) throw ValidationError("Invalid JSON body.");

    PermissionCreateInput input;
    input.name      = (*j).get("name",       "").asString();
    input.guardName = (*j).get("guard_name", "web").asString();
    input.method    = (*j).get("method",     "GET").asString();
    input.status    = (*j).get("status",     "Active").asString();
    input.desc      = (*j).get("desc",       "").asString();

    auto perm = co_await permSvc_->create(input, actorFromReq(req));
    Json::Value body;
    body["message"] = "Create Permission Success.";
    body["data"]    = perm.toJson();
    co_return jsonOk(std::move(body), drogon::k201Created);
}

drogon::Task<HttpResponsePtr> AccessApiController::permsUpdate(HttpRequestPtr req, std::string id) {
    auto j = req->getJsonObject();
    if (!j) throw ValidationError("Invalid JSON body.");

    PermissionUpdateInput input;
    input.name      = (*j).get("name",       "").asString();
    input.guardName = (*j).get("guard_name", "").asString();
    input.method    = (*j).get("method",     "").asString();
    input.status    = (*j).get("status",     "").asString();
    input.desc      = (*j).get("desc",       "").asString();

    auto perm = co_await permSvc_->update(id, input, actorFromReq(req));
    Json::Value body;
    body["message"] = "Update Permission Success.";
    body["data"]    = perm.toJson();
    co_return jsonOk(std::move(body));
}

drogon::Task<HttpResponsePtr> AccessApiController::permsDestroy(HttpRequestPtr req, std::string id) {
    co_await permSvc_->remove(id);
    Json::Value body;
    body["message"] = "Delete Permission Success.";
    body["data"]    = Json::nullValue;
    co_return jsonOk(std::move(body));
}
