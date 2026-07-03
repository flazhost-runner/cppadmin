#include "MediaController.h"
#include "../../../include/AppError.h"
#include <drogon/drogon.h>
#include <drogon/MultiPart.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <random>
#include <regex>
#include <set>

namespace fs = std::filesystem;

static const std::set<std::string> ALLOWED_EXT = {"jpg","jpeg","png","gif","webp","svg"};
static const size_t MAX_SIZE = 2 * 1024 * 1024; // 2 MB

std::filesystem::path MediaController::editorDir() {
    // Drogon upload path = <appRoot>/storage/uploads; editor subdir under uploads/editor
    fs::path base = drogon::app().getUploadPath();
    fs::path dir  = base / "editor";
    if (!fs::exists(dir)) fs::create_directories(dir);
    return dir;
}

drogon::HttpResponsePtr MediaController::jsonOk(Json::Value body, drogon::HttpStatusCode code) {
    body["status"] = true;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

drogon::HttpResponsePtr MediaController::jsonErr(const std::string &msg, drogon::HttpStatusCode code) {
    Json::Value body;
    body["status"]  = false;
    body["message"] = msg;
    body["data"]    = Json::nullValue;
    auto resp = drogon::HttpResponse::newHttpJsonResponse(std::move(body));
    resp->setStatusCode(code);
    return resp;
}

drogon::Task<drogon::HttpResponsePtr> MediaController::list(drogon::HttpRequestPtr req) {
    auto dir = editorDir();
    Json::Value files(Json::arrayValue);
    if (fs::exists(dir)) {
        for (const auto &entry : fs::directory_iterator(dir)) {
            if (!entry.is_regular_file()) continue;
            Json::Value f;
            f["name"] = entry.path().filename().string();
            f["url"]  = "/admin/v1/media/file/" + entry.path().filename().string();
            files.append(f);
        }
    }
    Json::Value body;
    body["message"] = "";
    body["data"]    = files;
    co_return jsonOk(std::move(body));
}

drogon::Task<drogon::HttpResponsePtr> MediaController::upload(drogon::HttpRequestPtr req) {
    drogon::MultiPartParser mpp;
    if (mpp.parse(req) != 0 || mpp.getFiles().empty()) {
        co_return jsonErr("No file uploaded.");
    }
    const auto &f = mpp.getFiles()[0];

    if (f.fileLength() > MAX_SIZE) {
        co_return jsonErr("File too large. Maximum 2MB.");
    }

    std::string origName = f.getFileName();
    std::string ext;
    auto dot = origName.rfind('.');
    if (dot != std::string::npos)
        ext = origName.substr(dot + 1);
    for (auto &c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    if (ALLOWED_EXT.find(ext) == ALLOWED_EXT.end()) {
        co_return jsonErr("File type not allowed. Use: jpg, png, gif, webp, svg.");
    }

    // Generate unique filename
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 rng(static_cast<uint64_t>(now));
    std::string unique = std::to_string(now) + "_" +
                         std::to_string(rng() & 0xFFFFFFFF);
    std::string filename = unique + "." + ext;

    auto dir  = editorDir();
    auto path = dir / filename;

    std::ofstream ofs(path, std::ios::binary);
    if (!ofs) {
        co_return jsonErr("Failed to save file.", drogon::k500InternalServerError);
    }
    ofs.write(f.fileData(), static_cast<std::streamsize>(f.fileLength()));
    ofs.close();

    Json::Value data;
    data["name"] = filename;
    data["url"]  = "/admin/v1/media/file/" + filename;
    data["key"]  = "editor/" + filename;

    Json::Value body;
    body["message"] = "File uploaded.";
    body["data"]    = data;
    co_return jsonOk(std::move(body), drogon::k201Created);
}

drogon::Task<drogon::HttpResponsePtr> MediaController::destroy(drogon::HttpRequestPtr req) {
    auto j = req->getJsonObject();
    std::string key;
    if (j && (*j).isMember("key")) {
        key = (*j)["key"].asString();
    } else {
        key = req->getParameter("key");
    }

    // Validate: only allow editor/<safe-filename>
    static const std::string prefix = "editor/";
    if (key.size() <= prefix.size() || key.substr(0, prefix.size()) != prefix) {
        co_return jsonErr("Invalid key.");
    }
    std::string name = key.substr(prefix.size());
    // Anti path-traversal
    if (name.find('/') != std::string::npos || name.find("..") != std::string::npos) {
        co_return jsonErr("Invalid key.");
    }

    auto path = editorDir() / name;
    if (fs::exists(path)) fs::remove(path);

    Json::Value body;
    body["message"] = "File deleted.";
    body["data"]    = Json::nullValue;
    co_return jsonOk(std::move(body));
}

drogon::Task<drogon::HttpResponsePtr> MediaController::file(drogon::HttpRequestPtr req, std::string name) {
    // Anti path-traversal
    if (name.find('/') != std::string::npos || name.find("..") != std::string::npos ||
        !std::regex_match(name, std::regex("^[A-Za-z0-9._-]+$"))) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k400BadRequest);
        co_return resp;
    }

    auto path = editorDir() / name;
    if (!fs::exists(path)) {
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setStatusCode(drogon::k404NotFound);
        co_return resp;
    }

    co_return drogon::HttpResponse::newFileResponse(path.string());
}
