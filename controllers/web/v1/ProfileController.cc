#include "ProfileController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
#include "../../../include/AppError.h"

using drogon_model::cppadmin::Users;

static std::string currentUid(const drogon::HttpRequestPtr &req) {
    auto attrs = req->getAttributes();
    if (!attrs->find("currentUser")) throw UnauthorizedError();
    const std::string &uid = attrs->get<std::string>("currentUser");
    if (uid.empty()) throw UnauthorizedError();
    return uid;
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::show(drogon::HttpRequestPtr req) {
    auto uid   = currentUid(req);
    auto user  = co_await userSvc_->findById(uid);
    auto roles = co_await userSvc_->rolesOf(uid);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data["user"] = user.toJson();

    Json::Value rArr(Json::arrayValue);
    for (const auto &r : roles) rArr.append(r.toJson());
    data["roles"] = rArr;

    co_return renderView("views::be::admin::profile::show", data);
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::edit(drogon::HttpRequestPtr req) {
    auto uid  = currentUid(req);
    auto user = co_await userSvc_->findById(uid);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data["user"] = user.toJson();
    co_return renderView("views::be::admin::profile::edit", data);
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::update(drogon::HttpRequestPtr req) {
    auto uid = currentUid(req);

    UserUpdateInput input;
    input.name     = req->getParameter("name");
    input.phone    = req->getParameter("phone");
    input.timezone = req->getParameter("timezone");
    input.picture  = req->getParameter("picture");

    co_await userSvc_->update(uid, input, uid);
    Flash::setSuccess(req, "Update Profile Success.");
    co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile");
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::editPassword(drogon::HttpRequestPtr req) {
    auto uid  = currentUid(req);
    auto user = co_await userSvc_->findById(uid);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data["user"] = user.toJson();
    co_return renderView("views::be::admin::profile::edit_password", data);
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::updatePassword(drogon::HttpRequestPtr req) {
    auto uid = currentUid(req);

    std::string current = req->getParameter("current_password");
    std::string newPass = req->getParameter("new_password");
    std::string confirm = req->getParameter("confirm_password");

    if (newPass != confirm) {
        Flash::setError(req, "Password confirmation does not match.");
        co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile/password");
    }
    if (newPass.size() < 8) {
        Flash::setError(req, "Password must be at least 8 characters.");
        co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile/password");
    }

    try {
        co_await userSvc_->changePassword(uid, current, newPass);
    } catch (const ValidationError &e) {
        Flash::setError(req, e.what());
        co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile/password");
    }

    Flash::setSuccess(req, "Update Profile Success.");
    co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile");
}
