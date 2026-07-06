#pragma once
#include "../include/FeTemplates.h"
#include <drogon/drogon.h>
#include <string>
#include <vector>

// Satu halaman hasil katalog FE (paginasi server-side).
struct FeCatalogPage {
    std::vector<fetpl::Item> datas;
    int totalData{0};
    int pageSize{12};
    int currentPage{1};
    int totalPages{0};
};

// Kontrak katalog template frontend (640 landing opentailwind).
struct IFeCatalogService {
    virtual ~IFeCatalogService() = default;

    // Seluruh katalog (memo + cache disk; fallback kurasi saat offline).
    virtual drogon::Task<std::vector<fetpl::Item>> list() = 0;

    // Daftar kategori unik (terurut) dari katalog.
    virtual drogon::Task<std::vector<std::string>> categories() = 0;

    // Filter (q_name atas name/slug, q_category exact) + paginasi; pinSlug
    // (template aktif) disematkan ke urutan pertama bila lolos filter.
    virtual drogon::Task<FeCatalogPage> paginate(std::string qName,
                                                 std::string qCategory,
                                                 int page,
                                                 int pageSize,
                                                 std::string pinSlug) = 0;

    // Apakah slug terdaftar di katalog.
    virtual drogon::Task<bool> has(std::string slug) = 0;

    // HTML preview 1 template (cache lokal dulu, lalu fetch upstream).
    // Throw AppError 400 bila slug tak dikenali, 502 bila fetch gagal.
    virtual drogon::Task<std::string> previewHtml(std::string slug) = 0;
};
