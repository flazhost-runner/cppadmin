#pragma once
#include <cctype>
#include <regex>
#include <string>
#include <vector>

// Katalog template frontend (landing) — kurasi dari opentailwind
// (https://github.com/lindoai/opentailwind, MIT). Mirror dari NodeAdmin
// src/config/feTemplates.ts. Tiap template self-contained (HTML + Tailwind v4
// CDN) dan di-download on-demand saat admin memilihnya (lihat FeTemplateService).
//
// Slug khusus 'default' merender view CSP landing lokal (views/fe/deflt,
// landing v6) alih-alih file HTML hasil unduhan.
namespace fetpl {

// Satu item katalog template frontend (landing) opentailwind.
struct Item {
    std::string slug;      // nama file opentailwind (tanpa .html) = id unik
    std::string name;      // nama tampil di switcher
    std::string category;  // kategori (untuk pengelompokan)
};

// Basis URL raw GitHub opentailwind untuk download on-demand.
inline constexpr const char *kBaseUrl =
    "https://raw.githubusercontent.com/lindoai/opentailwind/master/landings";

// GitHub API tree (recursive) untuk mendaftar seluruh 640 landing.
inline constexpr const char *kTreeUrl =
    "https://api.github.com/repos/lindoai/opentailwind/git/trees/master?recursive=1";

// Folder cache lokal (relatif root app). Berada di storage/ (folder runtime
// writable, sejajar storage/uploads) — BUKAN public/, sehingga file cache
// tidak pernah tersaji langsung sebagai static file publik.
inline constexpr const char *kDir         = "storage/fe/templates";
inline constexpr const char *kCatalogFile = "storage/fe/templates/_catalog.json";

// Slug khusus: render view CSP landing lokal (fe/deflt, landing v6).
inline constexpr const char *kDefaultView = "default";

// Template default aktif (sama dgn NodeAdmin DEFAULT_FE_TEMPLATE).
inline constexpr const char *kDefault = "agency-consulting-002-creative-agency";

// Pola slug opentailwind: `{kategori}-{NNN}-{nama}` (kategori boleh ber-hyphen,
// mis. `agency-consulting`). Dipakai validator (anti-SSRF: charset a-z0-9- +
// struktur tetap) & derive metadata.
inline const std::regex &slugRe() {
    static const std::regex re("^([a-z]+(?:-[a-z]+)*)-([0-9]{3})-([a-z0-9-]+)$");
    return re;
}

// Slug valid: 'default' (view lokal) atau cocok pola opentailwind.
inline bool isValidSlug(const std::string &slug) {
    if (slug.empty()) return false;
    if (slug == kDefaultView) return true;
    return std::regex_match(slug, slugRe());
}

// Title-case dari segmen hyphen: `digital-marketing` → `Digital Marketing`.
inline std::string titleize(const std::string &value) {
    std::string out;
    bool atWordStart = true;
    for (char c : value) {
        if (c == '-') {
            if (!out.empty() && out.back() != ' ') out += ' ';
            atWordStart = true;
            continue;
        }
        out += atWordStart ? static_cast<char>(std::toupper(static_cast<unsigned char>(c))) : c;
        atWordStart = false;
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

// Derive metadata tampil dari slug opentailwind. Bila slug tak cocok pola,
// pakai slug apa adanya sebagai name & kategori 'Other'.
inline Item derive(const std::string &slug) {
    std::smatch m;
    if (!std::regex_match(slug, m, slugRe())) return {slug, titleize(slug), "Other"};
    return {slug, titleize(m[3].str()), titleize(m[1].str())};
}

// Katalog kurasi (~15 dari 640 landing opentailwind) — fallback offline.
inline const std::vector<Item> &curated() {
    static const std::vector<Item> list = {
        {"agency-consulting-002-creative-agency", "Creative Agency", "Agency"},
        {"agency-consulting-001-digital-marketing-agency", "Digital Marketing Agency", "Agency"},
        {"technology-saas-001-hero-focused-conversion-page", "SaaS — Hero Focused", "Technology"},
        {"technology-saas-002-feature-rich-multi-section", "SaaS — Feature Rich", "Technology"},
        {"ecommerce-retail-001-fashion-boutique", "Fashion Boutique", "E-commerce"},
        {"ecommerce-retail-002-luxury-fashion-brand", "Luxury Fashion", "E-commerce"},
        {"portfolio-creative-001-creative-portfolio", "Creative Portfolio", "Portfolio"},
        {"portfolio-creative-002-minimal-portfolio", "Minimal Portfolio", "Portfolio"},
        {"professional-services-001-law-firm", "Law Firm", "Professional"},
        {"real-estate-property-001-real-estate-agency", "Real Estate Agency", "Real Estate"},
        {"food-hospitality-001-fine-dining-restaurant", "Fine Dining", "Food"},
        {"healthcare-wellness-001-family-doctor-clinic", "Family Clinic", "Healthcare"},
        {"education-training-001-private-school", "Private School", "Education"},
        {"fitness-sports-001-fitness-center", "Fitness Center", "Fitness"},
        {"travel-tourism-001-travel-agency", "Travel Agency", "Travel"},
    };
    return list;
}

}  // namespace fetpl
