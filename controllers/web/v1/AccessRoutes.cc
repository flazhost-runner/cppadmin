#include "AccessController.h"
#include "../../api/v1/AccessApiController.h"
#include "../../../services/UserService.h"
#include "../../../services/RoleService.h"
#include "../../../services/PermissionService.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <memory>

// Build a constraints vector with one HTTP method + all web filters
static std::vector<drogon::internal::HttpConstraint>
webConstraints(drogon::HttpMethod method) {
    return {
        method,
        std::string("MethodOverrideFilter"),
        std::string("CsrfFilter"),
        std::string("AuthFilter"),
        std::string("RbacFilter"),
    };
}

void registerAccessRoutes() {
    auto userSvc = std::make_shared<UserService>();
    auto roleSvc = std::make_shared<RoleService>();
    auto permSvc = std::make_shared<PermissionService>();
    auto ctrl    = std::make_shared<AccessController>(userSvc, roleSvc, permSvc);

    // AccessApiController extends HttpController — Drogon auto-instantiates it via default ctor.
    (void)std::make_shared<AccessApiController>(userSvc, roleSvc, permSvc);

    // ── Users ──────────────────────────────────────────────────────────────────
    ROUTE_REG("access.users.index",   "GET",    "/admin/v1/access/users");
    ROUTE_REG("access.users.create",  "GET",    "/admin/v1/access/users/create");
    ROUTE_REG("access.users.store",   "POST",   "/admin/v1/access/users");
    ROUTE_REG("access.users.show",    "GET",    "/admin/v1/access/users/{id}");
    ROUTE_REG("access.users.edit",    "GET",    "/admin/v1/access/users/{id}/edit");
    ROUTE_REG("access.users.update",  "PUT",    "/admin/v1/access/users/{id}");
    ROUTE_REG("access.users.delete",  "DELETE", "/admin/v1/access/users/{id}");

    drogon::app().registerHandler("/admin/v1/access/users",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->usersIndex(req)); },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/users/create",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->usersCreate(req)); },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/users",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->usersStore(req)); },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/users/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->usersShow(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/users/{id}/edit",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->usersEdit(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/users/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->usersUpdate(req, id);
        },
        webConstraints(drogon::Put));

    drogon::app().registerHandler("/admin/v1/access/users/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->usersDestroy(req, id);
        },
        webConstraints(drogon::Delete));

    // POST companion: dispatch PUT→Update, DELETE/other→Destroy (Drogon routes before MethodOverrideFilter)
    drogon::app().registerHandler("/admin/v1/access/users/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            auto m = req->getParameter("_method");
            std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            if (m == "PUT" || m == "PATCH")
                co_return co_await ctrl->usersUpdate(req, id);
            co_return co_await ctrl->usersDestroy(req, id);
        },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/users/delete_selected",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->usersDeleteSelected(req)); },
        webConstraints(drogon::Post));

    // ── Roles ──────────────────────────────────────────────────────────────────
    ROUTE_REG("access.roles.index",          "GET",    "/admin/v1/access/roles");
    ROUTE_REG("access.roles.create",         "GET",    "/admin/v1/access/roles/create");
    ROUTE_REG("access.roles.store",          "POST",   "/admin/v1/access/roles/store");
    ROUTE_REG("access.roles.delete_selected","POST",   "/admin/v1/access/roles/delete_selected");
    ROUTE_REG("access.roles.show",           "GET",    "/admin/v1/access/roles/{id}");
    ROUTE_REG("access.roles.edit",           "GET",    "/admin/v1/access/roles/{id}/edit");
    ROUTE_REG("access.roles.update",         "PUT",    "/admin/v1/access/roles/{id}");
    ROUTE_REG("access.roles.update_path",    "POST",   "/admin/v1/access/roles/{id}/update");
    ROUTE_REG("access.roles.delete_path",    "POST",   "/admin/v1/access/roles/{id}/delete");
    ROUTE_REG("access.roles.delete",         "DELETE", "/admin/v1/access/roles/{id}");

    drogon::app().registerHandler("/admin/v1/access/roles",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->rolesIndex(req)); },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/create",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->rolesCreate(req)); },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesShow(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/edit",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesEdit(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesUpdate(req, id);
        },
        webConstraints(drogon::Put));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesDestroy(req, id);
        },
        webConstraints(drogon::Delete));

    // Exact-path POST handlers registered BEFORE the {id} companion to prevent
    // Drogon's first-match-wins from swallowing them as id='store'/'delete_selected'.
    drogon::app().registerHandler("/admin/v1/access/roles/store",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->rolesStore(req)); },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/roles/delete_selected",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->rolesDeleteSelected(req)); },
        webConstraints(drogon::Post));

    // Path-segment update/delete routes (NodeAdmin spec: /{id}/update and /{id}/delete)
    drogon::app().registerHandler("/admin/v1/access/roles/{id}/update",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesUpdate(req, id);
        },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/delete",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesDestroy(req, id);
        },
        webConstraints(drogon::Post));

    // POST companion for legacy /{id}?_method=PUT|DELETE (kept for backward compat)
    drogon::app().registerHandler("/admin/v1/access/roles/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            auto m = req->getParameter("_method");
            std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            if (m == "PUT" || m == "PATCH")
                co_return co_await ctrl->rolesUpdate(req, id);
            co_return co_await ctrl->rolesDestroy(req, id);
        },
        webConstraints(drogon::Post));

    // ── Role → Permission management ──────────────────────────────────────────
    ROUTE_REG("access.roles.permission",                "GET",  "/admin/v1/access/roles/{id}/permission");
    ROUTE_REG("access.roles.permission.assign",         "GET",  "/admin/v1/access/roles/{id}/permission/{permId}/assign");
    ROUTE_REG("access.roles.permission.unassign",       "GET",  "/admin/v1/access/roles/{id}/permission/{permId}/unassign");
    ROUTE_REG("access.roles.permission.assign_selected",   "POST", "/admin/v1/access/roles/{id}/permission/assign_selected");
    ROUTE_REG("access.roles.permission.unassign_selected", "POST", "/admin/v1/access/roles/{id}/permission/unassign_selected");

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/permission",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesPermission(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/permission/{permId}/assign",
        [ctrl](drogon::HttpRequestPtr req, std::string id, std::string permId)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesPermissionAssign(req, id, permId);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/permission/{permId}/unassign",
        [ctrl](drogon::HttpRequestPtr req, std::string id, std::string permId)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesPermissionUnassign(req, id, permId);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/permission/assign_selected",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesPermissionAssignSelected(req, id);
        },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/roles/{id}/permission/unassign_selected",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->rolesPermissionUnassignSelected(req, id);
        },
        webConstraints(drogon::Post));

    // ── Permissions ────────────────────────────────────────────────────────────
    ROUTE_REG("access.permissions.index",   "GET",    "/admin/v1/access/permissions");
    ROUTE_REG("access.permissions.create",  "GET",    "/admin/v1/access/permissions/create");
    ROUTE_REG("access.permissions.store",   "POST",   "/admin/v1/access/permissions");
    ROUTE_REG("access.permissions.show",    "GET",    "/admin/v1/access/permissions/{id}");
    ROUTE_REG("access.permissions.edit",    "GET",    "/admin/v1/access/permissions/{id}/edit");
    ROUTE_REG("access.permissions.update",  "PUT",    "/admin/v1/access/permissions/{id}");
    ROUTE_REG("access.permissions.delete",  "DELETE", "/admin/v1/access/permissions/{id}");

    drogon::app().registerHandler("/admin/v1/access/permissions",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->permissionsIndex(req)); },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/permissions/create",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->permissionsCreate(req)); },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/permissions",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->permissionsStore(req)); },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/permissions/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->permissionsShow(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/permissions/{id}/edit",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->permissionsEdit(req, id);
        },
        webConstraints(drogon::Get));

    drogon::app().registerHandler("/admin/v1/access/permissions/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->permissionsUpdate(req, id);
        },
        webConstraints(drogon::Put));

    drogon::app().registerHandler("/admin/v1/access/permissions/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->permissionsDestroy(req, id);
        },
        webConstraints(drogon::Delete));

    // POST companion: dispatch PUT→Update, DELETE/other→Destroy (Drogon routes before MethodOverrideFilter)
    drogon::app().registerHandler("/admin/v1/access/permissions/{id}",
        [ctrl](drogon::HttpRequestPtr req, std::string id)
            -> drogon::Task<drogon::HttpResponsePtr> {
            auto m = req->getParameter("_method");
            std::transform(m.begin(), m.end(), m.begin(), ::toupper);
            if (m == "PUT" || m == "PATCH")
                co_return co_await ctrl->permissionsUpdate(req, id);
            co_return co_await ctrl->permissionsDestroy(req, id);
        },
        webConstraints(drogon::Post));

    drogon::app().registerHandler("/admin/v1/access/permissions/delete_selected",
        [ctrl](drogon::HttpRequestPtr req,
               std::function<void(const drogon::HttpResponsePtr &)> cb)
            -> drogon::Task<void> { cb(co_await ctrl->permissionsDeleteSelected(req)); },
        webConstraints(drogon::Post));
}
