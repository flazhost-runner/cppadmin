#pragma once
#include <drogon/drogon.h>
#include <filesystem>

class MediaController {
public:
    drogon::Task<drogon::HttpResponsePtr> list(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> upload(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> destroy(drogon::HttpRequestPtr req);
    drogon::Task<drogon::HttpResponsePtr> file(drogon::HttpRequestPtr req, std::string name);

private:
    static std::filesystem::path editorDir();
    static drogon::HttpResponsePtr jsonOk(Json::Value body, drogon::HttpStatusCode code = drogon::k200OK);
    static drogon::HttpResponsePtr jsonErr(const std::string &msg, drogon::HttpStatusCode code = drogon::k400BadRequest);
};
