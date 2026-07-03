#include "ProfileApiController.h"
#include "../../../include/AppError.h"
#include <drogon/HttpResponse.h>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpResponse;

drogon::HttpResponsePtr ProfileApiController::jsonOk(Json::Value body) {
    body["status"] = true;
    auto resp = HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(drogon::k200OK);
    return resp;
}

drogon::Task<HttpResponsePtr>
ProfileApiController::show(HttpRequestPtr req) {
    auto attrs = req->getAttributes();
    if (!attrs->find("currentUser")) throw UnauthorizedError();
    std::string uid = attrs->get<std::string>("currentUser");
    if (uid.empty()) throw UnauthorizedError();

    auto user  = co_await userSvc_->findById(uid);
    auto roles = co_await userSvc_->rolesOf(uid);

    Json::Value rArr(Json::arrayValue);
    for (const auto &r : roles) rArr.append(r.toJson());

    Json::Value body;
    body["message"]      = "";
    body["data"]         = user.toJson();
    body["data"]["roles"] = rArr;
    co_return jsonOk(std::move(body));
}
