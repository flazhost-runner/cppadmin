#pragma once
#include "ISettingService.h"
#include "FeTemplateService.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>

class SettingService : public ISettingService {
public:
    // Default: pakai FeTemplateService konkret (call-site lama tetap valid).
    SettingService() : feTemplate_(std::make_shared<FeTemplateService>()) {}
    explicit SettingService(std::shared_ptr<IFeTemplateService> feTemplate)
        : feTemplate_(std::move(feTemplate)) {}

    drogon::Task<drogon_model::cppadmin::Settings> findFirst() override;
    drogon::Task<drogon_model::cppadmin::Settings> update(
        const std::string &id, SettingUpdateInput input, const std::string &actorId) override;

private:
    std::shared_ptr<IFeTemplateService> feTemplate_;

    static std::mutex                                            cacheMu_;
    static std::optional<drogon_model::cppadmin::Settings>      cache_;
    static std::chrono::steady_clock::time_point                cacheAt_;
    static constexpr int                                         kTtlSec = 60;

    static void invalidate();
};
