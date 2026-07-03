#pragma once
#include <drogon/drogon.h>
#include "../../../services/IUserService.h"
#include "../../../services/IRoleService.h"
#include "../../../services/IPermissionService.h"
#include <memory>
#include <set>

class AccessController {
public:
    explicit AccessController(std::shared_ptr<IUserService>       userSvc,
                               std::shared_ptr<IRoleService>       roleSvc,
                               std::shared_ptr<IPermissionService> permSvc)
        : userSvc_(std::move(userSvc))
        , roleSvc_(std::move(roleSvc))
        , permSvc_(std::move(permSvc)) {}

    // Users
    drogon::Task<drogon::HttpResponsePtr> usersIndex(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> usersCreate(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> usersStore(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> usersShow(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> usersEdit(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> usersUpdate(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> usersDestroy(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> usersDeleteSelected(drogon::HttpRequestPtr req);

    // Roles
    drogon::Task<drogon::HttpResponsePtr> rolesIndex(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> rolesCreate(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> rolesStore(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> rolesShow(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesEdit(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesUpdate(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesDestroy(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesDeleteSelected(drogon::HttpRequestPtr req);

    // Role → Permission management
    drogon::Task<drogon::HttpResponsePtr> rolesPermission(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesPermissionAssign(drogon::HttpRequestPtr req, std::string id, std::string permId);
    drogon::Task<drogon::HttpResponsePtr> rolesPermissionUnassign(drogon::HttpRequestPtr req, std::string id, std::string permId);
    drogon::Task<drogon::HttpResponsePtr> rolesPermissionAssignSelected(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> rolesPermissionUnassignSelected(drogon::HttpRequestPtr req, std::string id);

    // Permissions
    drogon::Task<drogon::HttpResponsePtr> permissionsIndex(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> permissionsCreate(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> permissionsStore(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> permissionsShow(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> permissionsEdit(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> permissionsUpdate(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> permissionsDestroy(drogon::HttpRequestPtr req, std::string id);
    drogon::Task<drogon::HttpResponsePtr> permissionsDeleteSelected(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IUserService>       userSvc_;
    std::shared_ptr<IRoleService>       roleSvc_;
    std::shared_ptr<IPermissionService> permSvc_;

    std::string actorId(const drogon::HttpRequestPtr &req) const;
};
