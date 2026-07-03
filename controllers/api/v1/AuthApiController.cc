#include "AuthApiController.h"
#include "../../../include/AppError.h"
#include <drogon/HttpResponse.h>
#include "../../../include/AppError.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpResponse;

static HttpResponsePtr jsonOk(Json::Value body, drogon::HttpStatusCode code = drogon::k200OK) {
    auto resp = HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

// ── POST /api/v1/auth/login ───────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthApiController::login(HttpRequestPtr req) {
    auto body = req->getJsonObject();
    if (!body)
        throw ValidationError("Invalid JSON request body.");

    std::string email    = (*body).get("email",    "").asString();
    std::string password = (*body).get("password", "").asString();

    auto result = co_await auth_->login(email, password);

    Json::Value resp;
    resp["status"]  = true;
    resp["message"] = "Login Success.";
    Json::Value data;
    data["token"] = result.jwtToken;
    data["user"]  = result.user.toJson();
    resp["data"] = data;
    co_return jsonOk(std::move(resp));
}

// ── POST /api/v1/auth/register ────────────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthApiController::register_(HttpRequestPtr req) {
    auto body = req->getJsonObject();
    if (!body)
        throw ValidationError("Invalid JSON request body.");

    RegisterInput input;
    input.name     = (*body).get("name",     "").asString();
    input.email    = (*body).get("email",    "").asString();
    input.password = (*body).get("password", "").asString();
    input.phone    = (*body).get("phone",    "").asString();

    auto user = co_await auth_->registerUser(std::move(input));

    Json::Value resp;
    resp["status"] = true;
    resp["message"] = "Register Success.";
    resp["data"] = user.toJson();
    co_return jsonOk(std::move(resp), drogon::k201Created);
}

// ── POST /api/v1/auth/reset/request ──────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthApiController::resetReq(HttpRequestPtr req) {
    auto body = req->getJsonObject();
    if (!body)
        throw ValidationError("Invalid JSON request body.");

    ResetRequestInput input;
    input.email = (*body).get("email", "").asString();
    co_await auth_->requestPasswordReset(std::move(input));

    Json::Value resp;
    resp["status"]  = true;
    resp["message"] = "OTP Send Success.";
    resp["data"] = Json::nullValue;
    co_return jsonOk(std::move(resp));
}

// ── POST /api/v1/auth/reset/process ──────────────────────────────────────────
drogon::Task<HttpResponsePtr>
AuthApiController::resetProc(HttpRequestPtr req) {
    auto body = req->getJsonObject();
    if (!body)
        throw ValidationError("Invalid JSON request body.");

    ResetProcessInput input;
    input.email       = (*body).get("email",        "").asString();
    input.otp         = (*body).get("otp",          "").asString();
    input.newPassword = (*body).get("new_password", "").asString();
    co_await auth_->processPasswordReset(std::move(input));

    Json::Value resp;
    resp["status"]  = true;
    resp["message"] = "Reset Password Success.";
    resp["data"] = Json::nullValue;
    co_return jsonOk(std::move(resp));
}
