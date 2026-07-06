#pragma once
#include <drogon/drogon.h>
#include "../../../services/ISettingService.h"
#include "../../../services/IFeCatalogService.h"
#include <memory>

class SettingController {
public:
    SettingController(std::shared_ptr<ISettingService> svc,
                      std::shared_ptr<IFeCatalogService> catalog)
        : svc_(std::move(svc)), catalog_(std::move(catalog)) {}

    drogon::Task<drogon::HttpResponsePtr> edit(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> fePreview(drogon::HttpRequestPtr req,
                                                    std::string slug);

private:
    std::shared_ptr<ISettingService>   svc_;
    std::shared_ptr<IFeCatalogService> catalog_;
};
