#include "SettingController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
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
    data["sLogo"]        = sv(setting.getLogo());
    data["sFavicon"]     = sv(setting.getFavicon());
    data["sLoginImage"]  = sv(setting.getLoginImage());

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

    SettingUpdateInput input;
    input.name        = req->getParameter("name");
    input.description = req->getParameter("description");
    input.icon        = req->getParameter("icon");
    input.logo        = req->getParameter("logo");
    input.favicon     = req->getParameter("favicon");
    input.loginImage  = req->getParameter("login_image");
    input.phone       = req->getParameter("phone");
    input.address     = req->getParameter("address");
    input.email       = req->getParameter("email");
    input.copyright   = req->getParameter("copyright");
    input.theme       = req->getParameter("theme");
    input.feTemplate  = req->getParameter("fe_template");

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
