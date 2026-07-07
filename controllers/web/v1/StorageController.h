#pragma once
#include <drogon/drogon.h>
#include <string>

// Menyajikan berkas storage lokal pada prefix URL stabil "/storage/<key>".
// Hanya aktif untuk STORAGE_DRIVER=local; driver remote (oss/s3) memakai URL
// presigned absolut sehingga tak ada yang disajikan di sini (→ 404).
// Route publik (tanpa auth) agar gambar terender di halaman login & frontend.
class StorageController {
public:
    drogon::Task<drogon::HttpResponsePtr> serve(drogon::HttpRequestPtr req, std::string key);
};
