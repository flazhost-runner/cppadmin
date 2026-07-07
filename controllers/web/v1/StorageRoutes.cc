#include "StorageController.h"
#include <drogon/drogon.h>
#include <memory>
#include <vector>

// Penyajian berkas storage lokal di prefix "/storage/<key>".
// Route publik (hanya method GET, tanpa filter auth/CSRF/RBAC) supaya gambar
// terender di halaman publik (login, frontend) — sepadan dgn mount static
// "/storage" di NodeAdmin. Guard traversal ada di StorageController::serve.
void registerStorageRoutes() {
    auto ctrl = std::make_shared<StorageController>();

    std::vector<drogon::internal::HttpConstraint> get{drogon::Get};

    // (.*) menangkap key ber-nested (mis. "editor/169..._12345.webp").
    drogon::app().registerHandlerViaRegex(
        "/storage/(.*)",
        [ctrl](drogon::HttpRequestPtr req, std::string key)
            -> drogon::Task<drogon::HttpResponsePtr> {
            co_return co_await ctrl->serve(req, key);
        },
        get,
        "storage.serve");
}
