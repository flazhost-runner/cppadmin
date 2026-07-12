#include "SettingController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
#include "../../../include/helpers/FormHelper.h"
#include "../../../include/helpers/UploadHelper.h"
#include "../../../include/AppError.h"
#include "../../../include/FeTemplates.h"
#include <cstdlib>

drogon::Task<drogon::HttpResponsePtr>
SettingController::edit(drogon::HttpRequestPtr req) {
    auto setting = co_await svc_->findFirst();

    drogon::HttpViewData data;
    prepareViewData(data, req, setting.getValueOfTheme());

    auto sv = [](const std::string *p) -> std::string { return p ? *p : ""; };
    data["sName"]        = sv(setting.getName());
    data["sDescription"] = sv(setting.getDescription());
    data["sEmail"]       = sv(setting.getEmail());
    data["sPhone"]       = sv(setting.getPhone());
    data["sAddress"]     = sv(setting.getAddress());
    data["sCopyright"]   = sv(setting.getCopyright());
    data["sTheme"]       = setting.getValueOfTheme();
    data["sInitial"]     = sv(setting.getInitial());
    data["sIcon"]        = sv(setting.getIcon());
    data["sLogo"]        = upload::urlOf(sv(setting.getLogo()));
    data["sFavicon"]     = upload::urlOf(sv(setting.getFavicon()));
    data["sLoginImage"]  = upload::urlOf(sv(setting.getLoginImage()));

    // ── Katalog frontend template (640 landing opentailwind) ────────────────
    std::string feRaw = sv(setting.getFeTemplate());
    std::string feActive = fetpl::isValidSlug(feRaw) ? feRaw : fetpl::kDefault;
    data["sFeTemplate"] = feActive;

    std::string qName     = req->getParameter("q_name");
    std::string qCategory = req->getParameter("q_category");
    int qPage     = std::atoi(req->getParameter("q_page").c_str());
    int qPageSize = std::atoi(req->getParameter("q_page_size").c_str());
    if (qPage <= 0) qPage = 1;
    if (qPageSize <= 0) qPageSize = 12;

    // Template aktif disematkan (pin) ke halaman 1 agar admin langsung
    // melihat pilihan saat ini (paritas NodeAdmin SettingController.index).
    auto catalog = co_await catalog_->paginate(qName, qCategory, qPage, qPageSize, feActive);
    auto categories = co_await catalog_->categories();

    data["feCatalogItems"]      = catalog.datas;
    data["feCatalogTotal"]      = catalog.totalData;
    data["feCatalogPage"]       = catalog.currentPage;
    data["feCatalogPageSize"]   = catalog.pageSize;
    data["feCatalogTotalPages"] = catalog.totalPages;
    data["feCategories"]        = categories;
    data["feQName"]             = qName;
    data["feQCategory"]         = qCategory;

    co_return renderView("views::be::admin::setting::edit", data);
}

drogon::Task<drogon::HttpResponsePtr>
SettingController::update(drogon::HttpRequestPtr req) {
    auto setting = co_await svc_->findFirst();
    std::string id = setting.getValueOfId();

    // form::get, BUKAN req->getParameter(): form setting ber-enctype multipart
    // (logo/favicon/login_image adalah input file), dan getParameter() tidak mengurai
    // body multipart — semua field ini akan kosong. Karena SettingService melewati
    // field kosong, Save akan "berhasil" tanpa mengubah apa pun: sukses palsu.
    // Lihat include/helpers/FormHelper.h.
    SettingUpdateInput input;
    input.name        = form::get(req, "name");
    input.description = form::get(req, "description");
    input.icon        = form::get(req, "icon");
    input.phone       = form::get(req, "phone");
    input.address     = form::get(req, "address");
    input.email       = form::get(req, "email");
    input.copyright   = form::get(req, "copyright");
    input.theme       = form::get(req, "theme");
    input.feTemplate  = form::get(req, "fe_template");

    // Ketiganya input file, bukan teks — dibaca sebagai berkas terunggah.
    input.logo       = co_await upload::imageIfAny(req, "logo",        "settings/logo-");
    input.favicon    = co_await upload::imageIfAny(req, "favicon",     "settings/favicon-");
    input.loginImage = co_await upload::imageIfAny(req, "login_image", "settings/login-");

    auto sAttrs = req->getAttributes();
    std::string actor = sAttrs->find("currentUser") ? sAttrs->get<std::string>("currentUser") : "system";

    co_await svc_->update(id, input, actor);
    Flash::setSuccess(req, "Save Setting Success.");
    co_return drogon::HttpResponse::newRedirectionResponse("/admin/v1/setting");
}

// Preview 1 template FE (dikonsumsi via fetch() JS untuk thumbnail & modal).
// Error HARUS berupa status code polos (400/502), bukan flash+redirect ala
// exception handler web — redirect akan membuat fetch men-cache halaman login
// sebagai "template". Karena itu AppError ditangani lokal di sini.
drogon::Task<drogon::HttpResponsePtr>
SettingController::fePreview(drogon::HttpRequestPtr req, std::string slug) {
    (void)req;
    try {
        auto html = co_await catalog_->previewHtml(slug);
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setBody(std::move(html));
        resp->setContentTypeCodeAndCustomString(
            drogon::CT_TEXT_HTML, "text/html; charset=utf-8");
        co_return resp;
    } catch (const AppError &e) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(static_cast<drogon::HttpStatusCode>(e.statusCode));
        resp->setBody(e.what());
        resp->setContentTypeCodeAndCustomString(
            drogon::CT_TEXT_PLAIN, "text/plain; charset=utf-8");
        co_return resp;
    }
}
