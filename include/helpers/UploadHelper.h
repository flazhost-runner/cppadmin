#pragma once

#include <drogon/HttpRequest.h>
#include <drogon/utils/coroutine.h>
#include "AppError.h"
#include "FormHelper.h"
#include "Storage.h"
#include <chrono>
#include <map>
#include <random>
#include <string>

// Unggahan gambar dari form multipart — satu implementasi untuk profil, setting, dan
// user. Sebelumnya tidak ada satu pun: ketiga form punya <input type="file"> tetapi
// controller membacanya sebagai teks lewat getParameter(), sehingga berkas yang
// diunggah selalu dibuang tanpa jejak.
namespace upload {

inline constexpr size_t kMaxImageBytes = 2 * 1024 * 1024;  // 2 MB

inline const std::map<std::string, std::string> &imageMime() {
    static const std::map<std::string, std::string> m{
        {"jpg", "image/jpeg"}, {"jpeg", "image/jpeg"}, {"png", "image/png"},
        {"gif", "image/gif"},  {"webp", "image/webp"}, {"svg", "image/svg+xml"},
        {"ico", "image/x-icon"}};
    return m;
}

// URL tampil untuk nilai yang tersimpan di DB. Kolom gambar menyimpan KEY objek
// (mis. "avatars/x.png") — URL dibangun saat render agar benar untuk tiap driver
// (local → /storage/…, oss/s3 → presigned yang bisa kedaluwarsa, jadi TIDAK boleh
// ikut tersimpan di DB). Nilai lama yang sudah berupa URL/absolute path dibiarkan.
inline std::string urlOf(const std::string &stored) {
    if (stored.empty()) return "";
    if (stored.rfind("http://", 0) == 0 || stored.rfind("https://", 0) == 0 ||
        stored[0] == '/')
        return stored;
    return storage::url(stored);
}

/// Simpan gambar dari field `field` bila (dan hanya bila) user benar-benar memilih
/// berkas. Mengembalikan KEY objek, atau "" bila input file dibiarkan kosong —
/// service melewati field kosong, jadi gambar lama tidak tertimpa.
inline drogon::Task<std::string> imageIfAny(drogon::HttpRequestPtr req,
                                             std::string            field,
                                             std::string            keyPrefix) {
    std::string data, fileName;
    if (!form::file(req, field, data, fileName)) co_return std::string{};

    if (data.size() > kMaxImageBytes)
        throw ValidationError(field + ": file too large. Maximum 2MB.");

    std::string ext;
    auto dot = fileName.rfind('.');
    if (dot != std::string::npos) ext = fileName.substr(dot + 1);
    for (auto &c : ext) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));

    auto mime = imageMime().find(ext);
    if (mime == imageMime().end())
        throw ValidationError(field +
                              ": type not allowed. Use jpg, png, gif, webp, svg, ico.");

    auto now = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937_64 rng(static_cast<uint64_t>(now));
    std::string key = keyPrefix + std::to_string(now) + "-" +
                      std::to_string(rng() & 0xFFFFFFFF) + "." + ext;

    co_await storage::save(key, data, mime->second);
    co_return key;
}

}  // namespace upload
