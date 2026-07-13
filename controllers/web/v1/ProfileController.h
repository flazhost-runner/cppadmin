#pragma once
#include <drogon/drogon.h>
#include "../../../services/IUserService.h"
#include <memory>

class ProfileController {
public:
    explicit ProfileController(std::shared_ptr<IUserService> userSvc)
        : userSvc_(std::move(userSvc)) {}

    // Satu halaman form penuh (mirror NodeAdmin /admin/v1/profile): show merender
    // form, update memproses semua field + upload picture (local/oss/s3).
    drogon::Task<drogon::HttpResponsePtr> show(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> update(drogon::HttpRequestPtr req);

private:
    std::shared_ptr<IUserService> userSvc_;
};
