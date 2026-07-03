#pragma once
#include "../models/Settings.h"
#include <drogon/drogon.h>
#include <string>

struct SettingUpdateInput {
    std::string name;
    std::string description;
    std::string icon;
    std::string logo;
    std::string favicon;
    std::string loginImage;
    std::string phone;
    std::string address;
    std::string email;
    std::string copyright;
    std::string theme;
    std::string feTemplate;
};

struct ISettingService {
    virtual ~ISettingService() = default;

    virtual drogon::Task<drogon_model::cppadmin::Settings> findFirst() = 0;

    virtual drogon::Task<drogon_model::cppadmin::Settings> update(
        const std::string &id, SettingUpdateInput input, const std::string &actorId) = 0;
};
