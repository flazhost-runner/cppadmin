#include "SettingService.h"
#include "../include/AppError.h"
#include <drogon/orm/CoroMapper.h>

using drogon_model::cppadmin::Settings;
using drogon::orm::CoroMapper;
using drogon::orm::Criteria;
using drogon::orm::CompareOperator;

std::mutex                               SettingService::cacheMu_;
std::optional<Settings>                  SettingService::cache_;
std::chrono::steady_clock::time_point    SettingService::cacheAt_{};

void SettingService::invalidate() {
    std::lock_guard<std::mutex> lg(cacheMu_);
    cache_.reset();
}

drogon::Task<Settings> SettingService::findFirst() {
    {
        std::lock_guard<std::mutex> lg(cacheMu_);
        if (cache_.has_value()) {
            auto age = std::chrono::steady_clock::now() - cacheAt_;
            if (std::chrono::duration_cast<std::chrono::seconds>(age).count() < kTtlSec)
                co_return *cache_;
        }
    }

    auto db = drogon::app().getDbClient();
    CoroMapper<Settings> mapper(db);
    auto rows = co_await mapper.limit(1).findAll();
    if (rows.empty()) throw NotFoundError("Settings not configured.");
    Settings s = rows.front();

    {
        std::lock_guard<std::mutex> lg(cacheMu_);
        cache_   = s;
        cacheAt_ = std::chrono::steady_clock::now();
    }
    co_return s;
}

drogon::Task<Settings> SettingService::update(const std::string &id,
                                               SettingUpdateInput input,
                                               const std::string &actorId) {
    auto db = drogon::app().getDbClient();
    CoroMapper<Settings> mapper(db);

    auto rows = co_await mapper.findBy(
        Criteria(Settings::Cols::_id, CompareOperator::EQ, id));
    if (rows.empty()) throw NotFoundError("Settings not found.");

    Settings s = rows.front();
    if (!input.name.empty())        s.setName(input.name);
    if (!input.description.empty()) s.setDescription(input.description);
    if (!input.icon.empty())        s.setIcon(input.icon);
    if (!input.logo.empty())        s.setLogo(input.logo);
    if (!input.favicon.empty())     s.setFavicon(input.favicon);
    if (!input.loginImage.empty())  s.setLoginImage(input.loginImage);
    if (!input.phone.empty())       s.setPhone(input.phone);
    if (!input.address.empty())     s.setAddress(input.address);
    if (!input.email.empty())       s.setEmail(input.email);
    if (!input.copyright.empty())   s.setCopyright(input.copyright);
    if (!input.theme.empty())       s.setTheme(input.theme);
    if (!input.feTemplate.empty())  s.setFeTemplate(input.feTemplate);
    s.setUpdatedBy(actorId);

    co_await mapper.update(s);
    invalidate();
    co_return s;
}
