#pragma once
#include <drogon/drogon.h>

class ComponentsController {
public:
    drogon::Task<drogon::HttpResponsePtr> index(drogon::HttpRequestPtr req);
};
