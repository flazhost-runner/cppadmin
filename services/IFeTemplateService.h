#pragma once
#include <drogon/drogon.h>
#include <optional>
#include <string>

// Kontrak FeTemplateService (template frontend / landing switcher).
struct IFeTemplateService {
    virtual ~IFeTemplateService() = default;

    // Apakah file template sudah ada di cache lokal (storage/fe/templates).
    virtual bool isCached(const std::string &slug) = 0;

    // Slug valid: 'default' (view lokal) atau cocok pola opentailwind.
    virtual bool isValidSlug(const std::string &slug) = 0;

    // Slug template aktif dari nilai mentah settings.fe_template
    // (fallback default bila kosong/tak valid).
    virtual std::string getActiveSlug(const std::string &feTemplateRaw) = 0;

    // True bila slug = 'default' → dirender via view CSP lokal (landing v6),
    // bukan raw HTML.
    virtual bool isDefaultView(const std::string &slug) = 0;

    // Pastikan file template tersedia lokal — download dari opentailwind bila
    // perlu. Throw AppError bila slug tak valid / unduhan gagal.
    virtual drogon::Task<void> ensure(std::string slug) = 0;

    // HTML landing aktif (raw). std::nullopt bila template aktif = view
    // 'default' lokal atau tak ada HTML yang bisa disajikan (offline & belum
    // ter-cache) — pemanggil lalu merender landing fe/deflt (v6) sebagai
    // fallback aman.
    virtual drogon::Task<std::optional<std::string>> getActiveHtml(std::string feTemplateRaw) = 0;
};
