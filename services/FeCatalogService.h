#pragma once
#include "IFeCatalogService.h"
#include <chrono>
#include <mutex>

// Katalog template frontend (640 landing opentailwind). Sumber kebenaran =
// GitHub tree API, di-fetch SEKALI lalu di-cache (memori + file disk) agar tak
// membebani server/GitHub. Pencarian & paginasi diproses server-side di sini.
// Mirror dari NodeAdmin FeCatalogService.ts.
class FeCatalogService : public IFeCatalogService {
public:
    drogon::Task<std::vector<fetpl::Item>> list() override;
    drogon::Task<std::vector<std::string>> categories() override;
    drogon::Task<FeCatalogPage> paginate(std::string qName,
                                         std::string qCategory,
                                         int page,
                                         int pageSize,
                                         std::string pinSlug) override;
    drogon::Task<bool> has(std::string slug) override;
    drogon::Task<std::string> previewHtml(std::string slug) override;

private:
    // Memo lintas-instance (service dibuat per module Routes.cc).
    static std::mutex                             memoMu_;
    static std::vector<fetpl::Item>               memo_;
    static std::chrono::steady_clock::time_point  memoAt_;
};
