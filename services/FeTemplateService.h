#pragma once
#include "IFeTemplateService.h"

// Resolusi template frontend (landing) aktif + download/cache on-demand.
// Mirror dari NodeAdmin FeTemplateService.ts dengan semantik port
// (DotNetAdmin/DjangoAdmin/LaravelAdmin): slug 'default' → view lokal,
// slug opentailwind → raw HTML ter-cache di storage/fe/templates.
class FeTemplateService : public IFeTemplateService {
public:
    bool isCached(const std::string &slug) override;
    bool isValidSlug(const std::string &slug) override;
    std::string getActiveSlug(const std::string &feTemplateRaw) override;
    bool isDefaultView(const std::string &slug) override;
    drogon::Task<void> ensure(std::string slug) override;
    drogon::Task<std::optional<std::string>> getActiveHtml(std::string feTemplateRaw) override;
};
