#include "ComponentsController.h"
#include "../../../include/helpers/ViewHelper.h"

drogon::Task<drogon::HttpResponsePtr>
ComponentsController::index(drogon::HttpRequestPtr req) {
    drogon::HttpViewData data;
    prepareViewData(data, req);
    co_return renderView("views::be::admin::components::index", data);
}
