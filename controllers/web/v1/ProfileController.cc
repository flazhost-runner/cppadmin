#include "ProfileController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
#include "../../../include/helpers/FormHelper.h"
#include "../../../include/helpers/UploadHelper.h"
#include "../../../include/AppError.h"

using drogon_model::cppadmin::Users;

static std::string currentUid(const drogon::HttpRequestPtr &req) {
    auto attrs = req->getAttributes();
    if (!attrs->find("currentUser")) throw UnauthorizedError();
    const std::string &uid = attrs->get<std::string>("currentUser");
    if (uid.empty()) throw UnauthorizedError();
    return uid;
}

// View profil membaca currentUserName/Email/Picture, tetapi prepareViewData() hanya
// mengisi currentUserId saat user terautentikasi — ketiga kunci itu tak pernah ada,
// sehingga form edit selalu tampil KOSONG. Diisi eksplisit di sini dari user yang
// sudah dimuat.
static void injectCurrentUser(drogon::HttpViewData &data, const Users &user) {
    data["currentUserName"]  = user.getValueOfName();
    data["currentUserEmail"] = user.getValueOfEmail();
    const auto *pic = user.getPicture();  // nullable — tidak ada getValueOfPicture()
    data["currentUserPicture"] = upload::urlOf(pic ? *pic : std::string{});
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::show(drogon::HttpRequestPtr req) {
    auto uid   = currentUid(req);
    auto user  = co_await userSvc_->findById(uid);
    auto roles = co_await userSvc_->rolesOf(uid);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data["user"] = user.toJson();
    injectCurrentUser(data, user);

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
    injectCurrentUser(data, user);
    co_return renderView("views::be::admin::profile::edit", data);
}

drogon::Task<drogon::HttpResponsePtr>
ProfileController::update(drogon::HttpRequestPtr req) {
    auto uid = currentUid(req);

    // form::get, BUKAN req->getParameter(): form profil ber-enctype multipart
    // (ada input file), dan getParameter() tidak mengurai body multipart —
    // semua field ini akan kosong. Lihat include/helpers/FormHelper.h.
    UserUpdateInput input;
    input.name     = form::get(req, "name");
    input.phone    = form::get(req, "phone");
    input.timezone = form::get(req, "timezone");
    input.picture  = co_await upload::imageIfAny(req, "picture", "avatars/" + uid + "-");

    // blocked/blockedReason WAJIB dipertahankan apa adanya. UserService::update
    // menulis u.setBlocked(input.blocked) tanpa syarat, sedangkan form profil tidak
    // pernah mengirim field itu — nilai default `false` akan MEMBUKA BLOKIR.
    // Status blocked hanya diperiksa saat login (AuthService), bukan per-permintaan,
    // jadi user yang diblokir setelah login masih memegang JWT yang sah dan bisa
    // membuka blokir dirinya sendiri hanya dengan menyimpan profil. Ini hak admin,
    // bukan hak user atas dirinya sendiri.
    auto self = co_await userSvc_->findById(uid);
    input.blocked       = self.getValueOfBlocked();
    input.blockedReason = self.getBlockedReason() ? *self.getBlockedReason() : std::string{};

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
