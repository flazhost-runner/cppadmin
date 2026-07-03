#include "Settings.h"
#include <drogon/orm/Field.h>

namespace drogon_model { namespace cppadmin {

const std::string Settings::primaryKeyName = "id";

const std::string Settings::Cols::_id         = "id";
const std::string Settings::Cols::_name       = "name";
const std::string Settings::Cols::_theme      = "theme";
const std::string Settings::Cols::_created_by = "created_by";
const std::string Settings::Cols::_updated_by = "updated_by";

const std::vector<std::string> Settings::insertColumns_ = {
    "id","initial","name","description","icon","logo","favicon","login_image",
    "phone","address","email","copyright","theme","fe_template",
    "created_by","updated_by"
};

Settings::Settings(const drogon::orm::Row &r, const ssize_t /*indexOffset*/) noexcept {
    if (!r["id"].isNull())           id_          = std::make_shared<std::string>(r["id"].as<std::string>());
    if (!r["initial"].isNull())      initial_     = std::make_shared<std::string>(r["initial"].as<std::string>());
    if (!r["name"].isNull())         name_        = std::make_shared<std::string>(r["name"].as<std::string>());
    if (!r["description"].isNull())  description_ = std::make_shared<std::string>(r["description"].as<std::string>());
    if (!r["icon"].isNull())         icon_        = std::make_shared<std::string>(r["icon"].as<std::string>());
    if (!r["logo"].isNull())         logo_        = std::make_shared<std::string>(r["logo"].as<std::string>());
    if (!r["favicon"].isNull())      favicon_     = std::make_shared<std::string>(r["favicon"].as<std::string>());
    if (!r["login_image"].isNull())  loginImage_  = std::make_shared<std::string>(r["login_image"].as<std::string>());
    if (!r["phone"].isNull())        phone_       = std::make_shared<std::string>(r["phone"].as<std::string>());
    if (!r["address"].isNull())      address_     = std::make_shared<std::string>(r["address"].as<std::string>());
    if (!r["email"].isNull())        email_       = std::make_shared<std::string>(r["email"].as<std::string>());
    if (!r["copyright"].isNull())    copyright_   = std::make_shared<std::string>(r["copyright"].as<std::string>());
    if (!r["theme"].isNull())        theme_       = std::make_shared<std::string>(r["theme"].as<std::string>());
    if (!r["fe_template"].isNull())  feTemplate_  = std::make_shared<std::string>(r["fe_template"].as<std::string>());
    if (!r["created_by"].isNull())   createdBy_   = std::make_shared<std::string>(r["created_by"].as<std::string>());
    if (!r["updated_by"].isNull())   updatedBy_   = std::make_shared<std::string>(r["updated_by"].as<std::string>());
}

Settings::Settings(const Json::Value &j) noexcept(false) {
    if (j.isMember("id"))          id_          = std::make_shared<std::string>(j["id"].asString());
    if (j.isMember("initial"))     initial_     = std::make_shared<std::string>(j["initial"].asString());
    if (j.isMember("name"))        name_        = std::make_shared<std::string>(j["name"].asString());
    if (j.isMember("description")) description_ = std::make_shared<std::string>(j["description"].asString());
    if (j.isMember("icon"))        icon_        = std::make_shared<std::string>(j["icon"].asString());
    if (j.isMember("logo"))        logo_        = std::make_shared<std::string>(j["logo"].asString());
    if (j.isMember("favicon"))     favicon_     = std::make_shared<std::string>(j["favicon"].asString());
    if (j.isMember("login_image")) loginImage_  = std::make_shared<std::string>(j["login_image"].asString());
    if (j.isMember("phone"))       phone_       = std::make_shared<std::string>(j["phone"].asString());
    if (j.isMember("address"))     address_     = std::make_shared<std::string>(j["address"].asString());
    if (j.isMember("email"))       email_       = std::make_shared<std::string>(j["email"].asString());
    if (j.isMember("copyright"))   copyright_   = std::make_shared<std::string>(j["copyright"].asString());
    if (j.isMember("theme"))       theme_       = std::make_shared<std::string>(j["theme"].asString());
    if (j.isMember("fe_template")) feTemplate_  = std::make_shared<std::string>(j["fe_template"].asString());
    if (j.isMember("created_by"))  createdBy_   = std::make_shared<std::string>(j["created_by"].asString());
    if (j.isMember("updated_by"))  updatedBy_   = std::make_shared<std::string>(j["updated_by"].asString());
}

const std::vector<std::string> &Settings::insertColumns() noexcept { return insertColumns_; }

void Settings::outputArgs(drogon::orm::internal::SqlBinder &binder) const {
    if (id_)          binder << *id_;          else binder << nullptr;
    if (initial_)     binder << *initial_;     else binder << nullptr;
    if (name_)        binder << *name_;        else binder << nullptr;
    if (description_) binder << *description_; else binder << nullptr;
    if (icon_)        binder << *icon_;        else binder << nullptr;
    if (logo_)        binder << *logo_;        else binder << nullptr;
    if (favicon_)     binder << *favicon_;     else binder << nullptr;
    if (loginImage_)  binder << *loginImage_;  else binder << nullptr;
    if (phone_)       binder << *phone_;       else binder << nullptr;
    if (address_)     binder << *address_;     else binder << nullptr;
    if (email_)       binder << *email_;       else binder << nullptr;
    if (copyright_)   binder << *copyright_;   else binder << nullptr;
    if (theme_)       binder << *theme_;       else binder << nullptr;
    if (feTemplate_)  binder << *feTemplate_;  else binder << nullptr;
    if (createdBy_)   binder << *createdBy_;   else binder << nullptr;
    if (updatedBy_)   binder << *updatedBy_;   else binder << nullptr;
}

const std::vector<std::string> Settings::updateColumns() const {
    std::vector<std::string> cols;
    if (initial_)     cols.push_back("initial");
    if (name_)        cols.push_back("name");
    if (description_) cols.push_back("description");
    if (icon_)        cols.push_back("icon");
    if (logo_)        cols.push_back("logo");
    if (favicon_)     cols.push_back("favicon");
    if (loginImage_)  cols.push_back("login_image");
    if (phone_)       cols.push_back("phone");
    if (address_)     cols.push_back("address");
    if (email_)       cols.push_back("email");
    if (copyright_)   cols.push_back("copyright");
    if (theme_)       cols.push_back("theme");
    if (feTemplate_)  cols.push_back("fe_template");
    if (updatedBy_)   cols.push_back("updated_by");
    return cols;
}

void Settings::updateArgs(drogon::orm::internal::SqlBinder &binder) const {
    if (initial_)     binder << *initial_;
    if (name_)        binder << *name_;
    if (description_) binder << *description_;
    if (icon_)        binder << *icon_;
    if (logo_)        binder << *logo_;
    if (favicon_)     binder << *favicon_;
    if (loginImage_)  binder << *loginImage_;
    if (phone_)       binder << *phone_;
    if (address_)     binder << *address_;
    if (email_)       binder << *email_;
    if (copyright_)   binder << *copyright_;
    if (theme_)       binder << *theme_;
    if (feTemplate_)  binder << *feTemplate_;
    if (updatedBy_)   binder << *updatedBy_;
}

Json::Value Settings::toJson() const {
    Json::Value j;
    if (id_)          j["id"]          = *id_;
    if (initial_)     j["initial"]     = *initial_;
    if (name_)        j["name"]        = *name_;
    if (description_) j["description"] = *description_;
    if (icon_)        j["icon"]        = *icon_;
    if (logo_)        j["logo"]        = *logo_;
    if (favicon_)     j["favicon"]     = *favicon_;
    if (loginImage_)  j["login_image"] = *loginImage_;
    if (phone_)       j["phone"]       = *phone_;
    if (address_)     j["address"]     = *address_;
    if (email_)       j["email"]       = *email_;
    if (copyright_)   j["copyright"]   = *copyright_;
    if (theme_)       j["theme"]       = *theme_;
    if (feTemplate_)  j["fe_template"] = *feTemplate_;
    return j;
}

std::string Settings::sqlForFindingByPrimaryKey() {
    return "SELECT * FROM settings WHERE id = ?";
}
std::string Settings::sqlForDeletingByPrimaryKey() {
    return "DELETE FROM settings WHERE id = ?";
}
std::string Settings::sqlForInserting(bool &needSelection) const {
    needSelection = false;
    const auto &cols = insertColumns_;
    std::string sql = "INSERT INTO settings (";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) sql += ","; sql += cols[i]; }
    sql += ") VALUES (";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) sql += ","; sql += "?"; }
    sql += ")";
    return sql;
}

}} // namespace
