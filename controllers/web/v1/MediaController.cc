#include "MediaController.h"
#include "../../../include/AppError.h"
#include "../../../include/helpers/Storage.h"
#include <drogon/drogon.h>
#include <drogon/MultiPart.h>
#include <filesystem>
#include <chrono>
#include <random>
#include <set>

namespace fs = std::filesystem;

static const std::set<std::string> ALLOWED_EXT = {"jpg","jpeg","png","gif","webp","svg"};
static const size_t MAX_SIZE = 2 * 1024 * 1024; // 2 MB
static const std::string KEY_PREFIX = "editor/";

static std::string mimeFor(const std::string &ext) {
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "png")  return "image/png";
    if (ext == "gif")  return "image/gif";
    if (ext == "webp") return "image/webp";
    if (ext == "svg")  return "image/svg+xml";
    return "application/octet-stream";
}

// Direktori editor pada storage lokal (untuk listing driver=local).
std::filesystem::path MediaController::editorDir() {
    fs::path dir = storage::baseDir() / "editor";
    std::error_code ec;
    if (!fs::exists(dir, ec)) fs::create_directories(dir, ec);
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
    (void)req;
    Json::Value files(Json::arrayValue);
    // Listing hanya untuk driver lokal (menelusuri direktori). Driver remote
    // (oss/s3) butuh operasi LIST bertanda-tangan — di luar cakupan; kembalikan
    // daftar kosong (paritas fallback NodeAdmin saat storage tak bisa di-list).
    if (storage::isLocal()) {
        auto dir = editorDir();
        std::error_code ec;
        if (fs::exists(dir, ec)) {
            for (const auto &entry : fs::directory_iterator(dir)) {
                if (!entry.is_regular_file()) continue;
                std::string name = entry.path().filename().string();
                std::string key  = KEY_PREFIX + name;
                Json::Value f;
                f["name"] = name;
                f["key"]  = key;                      // dipakai tombol hapus filemanager
                f["url"]  = storage::url(key);        // URL render driver-aware
                files.append(f);
            }
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

    // Nama unik → key objek "editor/<unik>.<ext>". DB/HTML menyimpan key;
    // URL dibangun via storage::url() sesuai driver aktif.
    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 rng(static_cast<uint64_t>(now));
    std::string unique = std::to_string(now) + "_" + std::to_string(rng() & 0xFFFFFFFF);
    std::string filename = unique + "." + ext;
    std::string key = KEY_PREFIX + filename;

    try {
        co_await storage::save(key, std::string(f.fileData(), f.fileLength()), mimeFor(ext));
    } catch (const std::exception &e) {
        co_return jsonErr(std::string("Failed to save file. ") + e.what(),
                          drogon::k500InternalServerError);
    }

    Json::Value data;
    data["name"] = filename;
    data["key"]  = key;
    data["url"]  = storage::url(key);   // local → /storage/editor/…; oss/s3 → presigned

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

    // Validasi: hanya izinkan editor/<safe-filename>
    if (key.size() <= KEY_PREFIX.size() || key.substr(0, KEY_PREFIX.size()) != KEY_PREFIX) {
        co_return jsonErr("Invalid key.");
    }
    std::string name = key.substr(KEY_PREFIX.size());
    // Anti path-traversal
    if (name.find('/') != std::string::npos || name.find("..") != std::string::npos) {
        co_return jsonErr("Invalid key.");
    }

    co_await storage::remove(key);

    Json::Value body;
    body["message"] = "File deleted.";
    body["data"]    = Json::nullValue;
    co_return jsonOk(std::move(body));
}
