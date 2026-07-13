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

// GET /admin/v1/profile — satu form penuh selaras NodeAdmin (code/name/phone/email/
// timezone/password/status/picture). prepareViewData() hanya mengisi currentUserId
// saat login, jadi nilai form di-inject eksplisit dari user yang dimuat.
drogon::Task<drogon::HttpResponsePtr>
ProfileController::show(drogon::HttpRequestPtr req) {
    auto uid  = currentUid(req);
    auto user = co_await userSvc_->findById(uid);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",    std::string("Profile"));
    data.insert("userCode",     user.getValueOfCode());
    data.insert("userName",     user.getValueOfName());
    data.insert("userEmail",    user.getValueOfEmail());
    data.insert("userPhone",    user.getValueOfPhone());
    data.insert("userStatus",   user.getValueOfStatus());
    data.insert("userTimezone", user.getValueOfTimezone().empty()
                                    ? std::string{"UTC"} : user.getValueOfTimezone());
    // Kolom picture menyimpan KEY objek; URL dibangun saat render sesuai driver aktif
    // (local → /storage/…, oss/s3 → presigned).
    const auto *pic = user.getPicture();  // nullable — tak ada getValueOfPicture()
    data.insert("userPicture", upload::urlOf(pic ? *pic : std::string{}));

    co_return renderView("views::be::admin::profile::show", data);
}

// PUT /admin/v1/profile/update — memproses seluruh field + upload picture opsional
// (driver-aware local/oss/s3 via UploadHelper), lalu redirect ke dashboard (selaras
// NodeAdmin ProfileController.update). Memakai UserService::updateProfile yang TIDAK
// menyentuh roles/blocked — jadi user tak bisa membuka blokir dirinya lewat form ini.
drogon::Task<drogon::HttpResponsePtr>
ProfileController::update(drogon::HttpRequestPtr req) {
    auto uid = currentUid(req);

    // form::get, BUKAN req->getParameter(): form profil ber-enctype multipart (ada
    // input file), dan getParameter() tidak mengurai body multipart. Lihat FormHelper.h.
    std::string name     = form::get(req, "name");
    std::string email    = form::get(req, "email");
    std::string password = form::get(req, "password");
    std::string confirm  = form::get(req, "password_confirmation");

    if (name.empty() || email.empty()) {
        Flash::setError(req, "Name and email are required.");
        co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile");
    }
    if (!password.empty()) {
        if (password.size() < 8) {
            Flash::setError(req, "Password must be at least 8 characters.");
            co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile");
        }
        if (password != confirm) {
            Flash::setError(req, "Password & confirm password not match.");
            co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile");
        }
    }

    ProfileUpdateInput input;
    input.code     = form::get(req, "code");
    input.name     = name;
    input.email    = email;
    input.phone    = form::get(req, "phone");
    input.timezone = form::get(req, "timezone");
    input.status   = form::get(req, "status");
    input.password = password;
    try {
        // "" bila input file dibiarkan kosong → service melewati picture kosong,
        // foto lama tetap utuh.
        input.picture = co_await upload::imageIfAny(req, "picture", "avatars/" + uid + "-");
    } catch (const ValidationError &e) {
        Flash::setError(req, e.what());
        co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/profile");
    }

    co_await userSvc_->updateProfile(uid, input);
    Flash::setSuccess(req, "Update Profile Success.");
    co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/dashboard");
}
