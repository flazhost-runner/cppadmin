#pragma once
#include <drogon/HttpController.h>
#include "../../../services/IUserService.h"
#include "../../../services/UserService.h"
#include <memory>

class ProfileApiController : public drogon::HttpController<ProfileApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(ProfileApiController::show, "/api/v1/profile", drogon::Get, "AuthFilter");
    METHOD_LIST_END

    ProfileApiController()
        : userSvc_(std::make_shared<UserService>()) {}

    explicit ProfileApiController(std::shared_ptr<IUserService> userSvc)
        : userSvc_(std::move(userSvc)) {}

    drogon::Task<drogon::HttpResponsePtr> show(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IUserService> userSvc_;

    static drogon::HttpResponsePtr jsonOk(Json::Value body);
};
