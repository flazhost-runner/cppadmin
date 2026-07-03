#pragma once
#include "ISettingService.h"
#include <chrono>
#include <mutex>
#include <optional>

class SettingService : public ISettingService {
public:
    drogon::Task<drogon_model::cppadmin::Settings> findFirst() override;
    drogon::Task<drogon_model::cppadmin::Settings> update(
        const std::string &id, SettingUpdateInput input, const std::string &actorId) override;

private:
    static std::mutex                                            cacheMu_;
    static std::optional<drogon_model::cppadmin::Settings>      cache_;
    static std::chrono::steady_clock::time_point                cacheAt_;
    static constexpr int                                         kTtlSec = 60;

    static void invalidate();
};
