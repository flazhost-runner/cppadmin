#include "HomeController.h"
#include "../../../include/helpers/ViewHelper.h"

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;

static std::string sv(const std::string *p) { return p ? *p : ""; }

// Halaman home (frontend publik).
// - Slug aktif 'default' → dirender via view CSP lokal (fe/deflt, landing v6 —
//   head/header/footer terpisah + aset di public/fe/default).
// - Slug lain (hasil switch/download) → HTML mentah self-contained yang
//   ter-cache di storage/fe/templates (unduh on-demand bila belum ada). Bila
//   HTML tak tersedia (offline, gagal unduh) → fallback ke landing v6 lokal
//   agar halaman selalu tampil.
drogon::Task<HttpResponsePtr>
HomeController::index(HttpRequestPtr req) {
    drogon::HttpViewData data;
    injectAppSettings(data);

    std::string feRaw;
    try {
        auto setting = co_await svc_->findFirst();
        feRaw = sv(setting.getFeTemplate());
        data["settingName"]        = sv(setting.getName());
        data["settingLogo"]        = sv(setting.getLogo());
        data["settingInitial"]     = sv(setting.getInitial());
        data["settingDescription"] = sv(setting.getDescription());
        data["settingEmail"]       = sv(setting.getEmail());
        data["settingPhone"]       = sv(setting.getPhone());
        data["settingAddress"]     = sv(setting.getAddress());
        data["settingCopyright"]   = sv(setting.getCopyright());
        if (data.get<std::string>("appName").empty())
            data["appName"] = sv(setting.getName());
    } catch (...) {
        // Setting not found — render with defaults
        data["settingName"]    = std::string{"CppAdmin"};
        data["settingInitial"] = std::string{"Admin"};
    }

    auto slug = feTemplate_->getActiveSlug(feRaw);
    if (!feTemplate_->isDefaultView(slug)) {
        auto html = co_await feTemplate_->getActiveHtml(feRaw);
        if (html) {
            auto resp = drogon::HttpResponse::newHttpResponse();
            resp->setBody(std::move(*html));
            resp->setContentTypeCodeAndCustomString(
                drogon::CT_TEXT_HTML, "text/html; charset=utf-8");
            co_return resp;
        }
    }

    co_return renderView("views::fe::deflt::index", data);
}
