#pragma once
#include <drogon/drogon.h>
#include "../../../services/IUserService.h"
#include <memory>

class ProfileController {
public:
    explicit ProfileController(std::shared_ptr<IUserService> userSvc)
        : userSvc_(std::move(userSvc)) {}

    drogon::Task<drogon::HttpResponsePtr> show(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> edit(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> editPassword(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> updatePassword(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IUserService> userSvc_;
};
