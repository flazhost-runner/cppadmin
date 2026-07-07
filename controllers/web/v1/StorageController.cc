#include "StorageController.h"
#include "../../../include/helpers/Storage.h"
#include <filesystem>

namespace fs = std::filesystem;

static drogon::HttpResponsePtr statusOnly(drogon::HttpStatusCode code) {
    auto resp = drogon::HttpResponse::newHttpResponse();
    resp->setStatusCode(code);
    return resp;
}

drogon::Task<drogon::HttpResponsePtr>
StorageController::serve(drogon::HttpRequestPtr req, std::string key) {
    (void)req;

    // Driver remote menyajikan URL absolut presigned — tak ada penyajian lokal.
    if (!storage::isLocal()) co_return statusOnly(drogon::k404NotFound);

    // Anti path-traversal: tolak segmen kosong, absolut, atau mengandung "..".
    if (key.empty() || key.front() == '/' || key.find("..") != std::string::npos) {
        co_return statusOnly(drogon::k400BadRequest);
    }

    // Pastikan hasil resolusi tetap di dalam base dir (pertahanan berlapis).
    fs::path base = fs::weakly_canonical(storage::baseDir());
    fs::path path = fs::weakly_canonical(base / key);
    std::string baseStr = base.string();
    if (!baseStr.empty() && baseStr.back() != fs::path::preferred_separator)
        baseStr += fs::path::preferred_separator;
    if (path.string().compare(0, baseStr.size(), baseStr) != 0) {
        co_return statusOnly(drogon::k403Forbidden);
    }

    std::error_code ec;
    if (!fs::exists(path, ec) || fs::is_directory(path, ec)) {
        co_return statusOnly(drogon::k404NotFound);
    }

    // newFileResponse menetapkan Content-Type dari ekstensi secara otomatis.
    co_return drogon::HttpResponse::newFileResponse(path.string());
}
