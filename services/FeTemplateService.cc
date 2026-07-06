#include "FeTemplateService.h"
#include "../include/AppError.h"
#include "../include/FeTemplates.h"
#include "../include/helpers/HttpFetch.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

// Timeout download 1 file HTML template (detik).
constexpr long kFetchTimeoutSec = 15;

// storage/fe/templates — diturunkan dari uploadPath (appRoot/storage/uploads,
// di-set absolut di main.cc) agar konsisten dgn path storage lain & bebas CWD.
std::filesystem::path feDir() {
    std::filesystem::path uploads(drogon::app().getUploadPath());
    return uploads.parent_path() / "fe" / "templates";
}

std::filesystem::path feFile(const std::string &slug) {
    return feDir() / (slug + ".html");
}

// Cek isi mengandung `</html>` (case-insensitive) — validasi HTML utuh.
bool looksLikeHtml(const std::string &html) {
    std::string lower(html);
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lower.find("</html>") != std::string::npos;
}

std::optional<std::string> readFile(const std::filesystem::path &p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

}  // namespace

bool FeTemplateService::isCached(const std::string &slug) {
    return std::filesystem::exists(feFile(slug));
}

bool FeTemplateService::isValidSlug(const std::string &slug) {
    return fetpl::isValidSlug(slug);
}

std::string FeTemplateService::getActiveSlug(const std::string &feTemplateRaw) {
    return isValidSlug(feTemplateRaw) ? feTemplateRaw : fetpl::kDefault;
}

bool FeTemplateService::isDefaultView(const std::string &slug) {
    return slug == fetpl::kDefaultView;
}

// Pastikan template tersedia lokal. Bila belum → download HTML dari
// opentailwind (GitHub raw) lalu simpan ke folder cache. Hanya slug yang
// cocok pola opentailwind yang diizinkan (anti SSRF/arbitrary fetch).
drogon::Task<void> FeTemplateService::ensure(std::string slug) {
    if (!isValidSlug(slug)) throw AppError("Template tidak dikenali", 400);
    if (isDefaultView(slug) || isCached(slug)) co_return;

    auto res = co_await httpGetCoro(
        std::string(fetpl::kBaseUrl) + "/" + slug + ".html", {}, kFetchTimeoutSec);
    if (!res.error.empty()) {
        throw AppError("Gagal mengunduh template: " + res.error, 502);
    }
    if (res.status != 200) {
        throw AppError("Gagal mengunduh template: HTTP " + std::to_string(res.status), 502);
    }
    std::string html = std::move(res.body);

    if (!looksLikeHtml(html)) throw AppError("Template terunduh tidak valid", 502);

    std::error_code ec;
    std::filesystem::create_directories(feDir(), ec);
    std::ofstream out(feFile(slug), std::ios::binary | std::ios::trunc);
    if (!out) throw AppError("Gagal menyimpan template ke cache", 500);
    out << html;
}

drogon::Task<std::optional<std::string>>
FeTemplateService::getActiveHtml(std::string feTemplateRaw) {
    auto slug = getActiveSlug(feTemplateRaw);
    if (isDefaultView(slug)) co_return std::nullopt;

    // Download on-demand (best-effort) — instalasi baru langsung menyajikan
    // template default opentailwind begitu jaringan memungkinkan.
    if (!isCached(slug)) {
        try {
            co_await ensure(slug);
        } catch (const AppError &e) {
            // offline / gagal unduh → jatuh ke fallback di bawah
            LOG_WARN << "FE template '" << slug << "' tak tersedia: " << e.what();
        }
    }

    std::string target = isCached(slug) ? slug : fetpl::kDefault;
    auto html = readFile(feFile(target));
    if (!html) co_return std::nullopt;
    co_return html;
}
