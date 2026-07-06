#include "FeCatalogService.h"
#include "../include/AppError.h"
#include "../include/helpers/HttpFetch.h"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <json/json.h>
#include <set>
#include <sstream>

namespace {

// TTL cache memori katalog. Disk dipakai sebagai persist lintas-restart.
constexpr auto kCatalogTtl = std::chrono::hours(6);

// Timeout fetch preview 1 file HTML (detik) — cukup ketat, file tunggal & ringan.
constexpr long kFetchTimeoutSec = 8;

// Timeout fetch tree katalog (detik) — lebih longgar dari preview: respons tree
// recursive mencakup 640 entry (lebih besar) & hanya dijalankan SEKALI lalu
// di-cache (memori+disk). Longgar agar blip jaringan tak men-degrade ke
// fallback kurasi (15 item) yang membuat katalog tampak nyaris kosong.
constexpr long kTreeFetchTimeoutSec = 20;

// storage/fe — diturunkan dari uploadPath (appRoot/storage/uploads) agar
// absolut & konsisten dgn FeTemplateService tanpa bergantung CWD.
std::filesystem::path storageFeDir() {
    std::filesystem::path uploads(drogon::app().getUploadPath());
    return uploads.parent_path() / "fe" / "templates";
}

std::filesystem::path catalogFile() { return storageFeDir() / "_catalog.json"; }

std::filesystem::path localHtmlFile(const std::string &slug) {
    return storageFeDir() / (slug + ".html");
}

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return s;
}

bool containsCi(const std::string &haystack, const std::string &needle) {
    return toLower(haystack).find(toLower(needle)) != std::string::npos;
}

bool looksLikeHtml(const std::string &html) {
    return toLower(html).find("</html>") != std::string::npos;
}

std::optional<std::string> readFile(const std::filesystem::path &p) {
    std::ifstream in(p, std::ios::binary);
    if (!in) return std::nullopt;
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Urut stabil: kategori lalu nama.
void sortItems(std::vector<fetpl::Item> &items) {
    std::sort(items.begin(), items.end(), [](const fetpl::Item &a, const fetpl::Item &b) {
        if (a.category != b.category) return a.category < b.category;
        return a.name < b.name;
    });
}

// Parse path tree → item landing (buang prefix `landings/` & `.html`).
std::vector<fetpl::Item> parseTree(const Json::Value &tree) {
    static const std::string prefix = "landings/";
    static const std::string suffix = ".html";
    std::vector<fetpl::Item> items;
    for (const auto &node : tree) {
        if (!node.isObject()) continue;
        if (node.get("type", "").asString() != "blob") continue;
        std::string path = node.get("path", "").asString();
        if (path.rfind(prefix, 0) != 0 || path.size() <= prefix.size() + suffix.size()) continue;
        if (path.compare(path.size() - suffix.size(), suffix.size(), suffix) != 0) continue;
        items.push_back(fetpl::derive(
            path.substr(prefix.size(), path.size() - prefix.size() - suffix.size())));
    }
    sortItems(items);
    return items;
}

std::vector<fetpl::Item> readDiskCache() {
    auto raw = readFile(catalogFile());
    if (!raw) return {};
    Json::Value data;
    Json::CharReaderBuilder rb;
    std::string errs;
    std::istringstream in(*raw);
    if (!Json::parseFromStream(rb, in, &data, &errs) || !data.isArray()) return {};
    std::vector<fetpl::Item> items;
    for (const auto &n : data) {
        if (!n.isObject()) continue;
        fetpl::Item it{n.get("slug", "").asString(), n.get("name", "").asString(),
                       n.get("category", "").asString()};
        if (!it.slug.empty()) items.push_back(std::move(it));
    }
    return items;
}

void writeDiskCache(const std::vector<fetpl::Item> &items) {
    // Cache disk best-effort — kegagalan tulis tak menggagalkan list().
    std::error_code ec;
    std::filesystem::create_directories(storageFeDir(), ec);
    if (ec) return;
    Json::Value arr(Json::arrayValue);
    for (const auto &it : items) {
        Json::Value n;
        n["slug"] = it.slug;
        n["name"] = it.name;
        n["category"] = it.category;
        arr.append(n);
    }
    std::ofstream out(catalogFile(), std::ios::trunc);
    if (!out) return;
    Json::StreamWriterBuilder wb;
    wb["indentation"] = "";
    out << Json::writeString(wb, arr);
}

}  // namespace

std::mutex                            FeCatalogService::memoMu_;
std::vector<fetpl::Item>              FeCatalogService::memo_;
std::chrono::steady_clock::time_point FeCatalogService::memoAt_{};

drogon::Task<std::vector<fetpl::Item>> FeCatalogService::list() {
    {
        std::lock_guard<std::mutex> lk(memoMu_);
        if (!memo_.empty() && std::chrono::steady_clock::now() - memoAt_ < kCatalogTtl)
            co_return memo_;
    }

    auto disk = readDiskCache();
    if (!disk.empty()) {
        std::lock_guard<std::mutex> lk(memoMu_);
        memo_ = disk;
        memoAt_ = std::chrono::steady_clock::now();
        co_return disk;
    }

    // Belum ada cache → fetch GitHub tree sekali. (Tanpa gate lintas-coroutine:
    // request paralel terburuk fetch ganda yang idempoten, lalu ter-memo.)
    std::vector<fetpl::Item> data;
    auto res = co_await httpGetCoro(fetpl::kTreeUrl,
                                    {"Accept: application/vnd.github+json"},
                                    kTreeFetchTimeoutSec);
    try {
        if (!res.error.empty()) throw std::runtime_error(res.error);
        if (res.status != 200)
            throw std::runtime_error("HTTP " + std::to_string(res.status));

        Json::Value body;
        Json::CharReaderBuilder rb;
        std::string errs;
        std::istringstream in(res.body);
        if (!Json::parseFromStream(rb, in, &body, &errs))
            throw std::runtime_error("JSON tidak valid");
        data = parseTree(body["tree"]);
        if (data.empty()) throw std::runtime_error("katalog kosong");
    } catch (const std::exception &e) {
        // Fallback ke katalog kurasi agar UI tetap berfungsi offline.
        LOG_ERROR << "Fetch katalog opentailwind gagal, pakai fallback kurasi: "
                  << e.what();
        auto fallback = fetpl::curated();
        std::lock_guard<std::mutex> lk(memoMu_);
        memo_ = fallback;
        memoAt_ = std::chrono::steady_clock::now();
        co_return fallback;
    }

    {
        std::lock_guard<std::mutex> lk(memoMu_);
        memo_ = data;
        memoAt_ = std::chrono::steady_clock::now();
    }
    writeDiskCache(data);
    co_return data;
}

drogon::Task<std::vector<std::string>> FeCatalogService::categories() {
    auto all = co_await list();
    std::set<std::string> uniq;
    for (const auto &t : all) uniq.insert(t.category);
    co_return std::vector<std::string>(uniq.begin(), uniq.end());
}

drogon::Task<FeCatalogPage> FeCatalogService::paginate(std::string qName,
                                                       std::string qCategory,
                                                       int page,
                                                       int pageSize,
                                                       std::string pinSlug) {
    auto all = co_await list();

    std::vector<fetpl::Item> filtered;
    for (const auto &t : all) {
        bool okName = qName.empty() || containsCi(t.name, qName) || containsCi(t.slug, qName);
        bool okCat = qCategory.empty() || t.category == qCategory;
        if (okName && okCat) filtered.push_back(t);
    }

    // Sematkan template aktif ke paling depan (bila lolos filter) agar tampil
    // di halaman pertama — memudahkan admin melihat pilihan saat ini.
    if (!pinSlug.empty()) {
        auto it = std::find_if(filtered.begin(), filtered.end(),
                               [&](const fetpl::Item &t) { return t.slug == pinSlug; });
        if (it != filtered.end() && it != filtered.begin()) {
            fetpl::Item pinned = *it;
            filtered.erase(it);
            filtered.insert(filtered.begin(), std::move(pinned));
        }
    }

    FeCatalogPage result;
    result.pageSize = pageSize > 0 ? pageSize : 12;
    result.currentPage = page > 0 ? page : 1;
    result.totalData = static_cast<int>(filtered.size());
    result.totalPages =
        (result.totalData + result.pageSize - 1) / result.pageSize;

    auto start = static_cast<size_t>(result.currentPage - 1) *
                 static_cast<size_t>(result.pageSize);
    for (size_t i = start;
         i < filtered.size() && i < start + static_cast<size_t>(result.pageSize); ++i) {
        result.datas.push_back(filtered[i]);
    }
    co_return result;
}

drogon::Task<bool> FeCatalogService::has(std::string slug) {
    auto all = co_await list();
    co_return std::any_of(all.begin(), all.end(),
                          [&](const fetpl::Item &t) { return t.slug == slug; });
}

drogon::Task<std::string> FeCatalogService::previewHtml(std::string slug) {
    if (!(co_await has(slug))) throw AppError("Template tidak dikenali", 400);

    // 1) Cache lokal lebih dulu — instan & tak bergantung jaringan/rate-limit.
    auto readLocal = [&]() -> std::optional<std::string> {
        auto html = readFile(localHtmlFile(slug));
        if (html && looksLikeHtml(*html)) return html;
        return std::nullopt;
    };
    if (auto local = readLocal()) co_return *local;

    // 2) Fetch upstream dengan timeout agar tak menggantung saat GitHub lambat.
    auto res = co_await httpGetCoro(std::string(fetpl::kBaseUrl) + "/" + slug + ".html",
                                    {}, kFetchTimeoutSec);
    try {
        if (!res.error.empty()) throw std::runtime_error(res.error);
        if (res.status != 200)
            throw std::runtime_error("HTTP " + std::to_string(res.status));
        if (!looksLikeHtml(res.body)) throw std::runtime_error("HTML tidak valid");
        co_return std::move(res.body);
    } catch (const std::exception &e) {
        // 3) Fallback terakhir: cache lokal (jika sempat ter-download sebagian).
        if (auto fallback = readLocal()) co_return *fallback;
        throw AppError(std::string("Gagal mengambil preview: ") + e.what(), 502);
    }
}
