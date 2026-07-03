#pragma once
#include <drogon/HttpController.h>
#include "../../../services/IUserService.h"
#include "../../../services/IRoleService.h"
#include "../../../services/IPermissionService.h"
#include "../../../services/UserService.h"
#include "../../../services/RoleService.h"
#include "../../../services/PermissionService.h"
#include "../../../include/AppError.h"
#include <memory>

class AccessApiController : public drogon::HttpController<AccessApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(AccessApiController::usersIndex,   "/api/v1/access/users",         drogon::Get,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::usersShow,    "/api/v1/access/users/{id}",    drogon::Get,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::usersStore,   "/api/v1/access/users",         drogon::Post,   "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::usersUpdate,  "/api/v1/access/users/{id}",    drogon::Put,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::usersDestroy, "/api/v1/access/users/{id}",    drogon::Delete, "AuthFilter","RbacFilter");

    ADD_METHOD_TO(AccessApiController::rolesIndex,   "/api/v1/access/roles",         drogon::Get,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::rolesShow,    "/api/v1/access/roles/{id}",    drogon::Get,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::rolesStore,   "/api/v1/access/roles",         drogon::Post,   "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::rolesUpdate,  "/api/v1/access/roles/{id}",    drogon::Put,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::rolesDestroy, "/api/v1/access/roles/{id}",    drogon::Delete, "AuthFilter","RbacFilter");

    ADD_METHOD_TO(AccessApiController::permsIndex,   "/api/v1/access/permissions",      drogon::Get,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::permsStore,   "/api/v1/access/permissions",      drogon::Post,   "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::permsShow,    "/api/v1/access/permissions/{id}", drogon::Get,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::permsUpdate,  "/api/v1/access/permissions/{id}", drogon::Put,    "AuthFilter","RbacFilter");
    ADD_METHOD_TO(AccessApiController::permsDestroy, "/api/v1/access/permissions/{id}", drogon::Delete, "AuthFilter","RbacFilter");
    METHOD_LIST_END

    // Default ctor for Drogon auto-instantiation (Drogon 1.8.7 — no registerObject)
    AccessApiController()
        : userSvc_(std::make_shared<UserService>())
        , roleSvc_(std::make_shared<RoleService>())
        , permSvc_(std::make_shared<PermissionService>()) {}

    // Explicit ctor for test injection
    explicit AccessApiController(std::shared_ptr<IUserService>       userSvc,
                                  std::shared_ptr<IRoleService>       roleSvc,
                                  std::shared_ptr<IPermissionService> permSvc)
        : userSvc_(std::move(userSvc))
        , roleSvc_(std::move(roleSvc))
        , permSvc_(std::move(permSvc)) {}

    drogon::Task<drogon::HttpResponsePtr> usersIndex(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> usersShow(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> usersStore(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> usersUpdate(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> usersDestroy(drogon::HttpRequestPtr req, std::string id);

    drogon::Task<drogon::HttpResponsePtr> rolesIndex(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> rolesShow(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesStore(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> rolesUpdate(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesDestroy(drogon::HttpRequestPtr req, std::string id);

    drogon::Task<drogon::HttpResponsePtr> permsIndex(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> permsShow(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> permsStore(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> permsUpdate(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> permsDestroy(drogon::HttpRequestPtr req, std::string id);

private:
    std::shared_ptr<IUserService>       userSvc_;
    std::shared_ptr<IRoleService>       roleSvc_;
    std::shared_ptr<IPermissionService> permSvc_;

    static drogon::HttpResponsePtr jsonOk(Json::Value body,
        drogon::HttpStatusCode code = drogon::k200OK);
};
