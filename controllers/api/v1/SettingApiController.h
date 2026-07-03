#pragma once
#include <drogon/HttpController.h>
#include "../../../services/ISettingService.h"
#include "../../../services/SettingService.h"
#include <memory>

class SettingApiController : public drogon::HttpController<SettingApiController> {
public:
    METHOD_LIST_BEGIN
    ADD_METHOD_TO(SettingApiController::show, "/api/v1/setting", drogon::Get, "AuthFilter");
    METHOD_LIST_END

    SettingApiController()
        : svc_(std::make_shared<SettingService>()) {}

    explicit SettingApiController(std::shared_ptr<ISettingService> svc)
        : svc_(std::move(svc)) {}

    drogon::Task<drogon::HttpResponsePtr> show(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<ISettingService> svc_;

    static drogon::HttpResponsePtr jsonOk(Json::Value body);
};
