#include "Users.h"
#include <drogon/orm/Field.h>
#include <stdexcept>

namespace drogon_model { namespace cppadmin {

const std::string Users::primaryKeyName = "id";

// Cols constants
const std::string Users::Cols::_id         = "id";
const std::string Users::Cols::_code       = "code";
const std::string Users::Cols::_name       = "name";
const std::string Users::Cols::_phone      = "phone";
const std::string Users::Cols::_email      = "email";
const std::string Users::Cols::_password   = "password";
const std::string Users::Cols::_status     = "status";
const std::string Users::Cols::_timezone   = "timezone";
const std::string Users::Cols::_blocked    = "blocked";
const std::string Users::Cols::_picture    = "picture";
const std::string Users::Cols::_created_by = "created_by";
const std::string Users::Cols::_updated_by = "updated_by";

// insertColumns_ must match outputArgs() order.
// Excludes email_verified_at, created_at, updated_at (DB-defaulted).
const std::vector<std::string> Users::insertColumns_ = {
    "id","code","name","phone","email","password",
    "password_otp","password_otp_expires","status","picture",
    "blocked","blocked_reason","timezone","created_by","updated_by"
};

// Use named column access (r["col"]) so positions never go stale.
Users::Users(const drogon::orm::Row &r, const ssize_t /*indexOffset*/) noexcept {
    if (!r["id"].isNull())                   id_                 = std::make_shared<std::string>(r["id"].as<std::string>());
    if (!r["code"].isNull())                 code_               = std::make_shared<std::string>(r["code"].as<std::string>());
    if (!r["name"].isNull())                 name_               = std::make_shared<std::string>(r["name"].as<std::string>());
    if (!r["phone"].isNull())                phone_              = std::make_shared<std::string>(r["phone"].as<std::string>());
    if (!r["email"].isNull())                email_              = std::make_shared<std::string>(r["email"].as<std::string>());
    if (!r["password"].isNull())             password_           = std::make_shared<std::string>(r["password"].as<std::string>());
    if (!r["password_otp"].isNull())         passwordOtp_        = std::make_shared<std::string>(r["password_otp"].as<std::string>());
    if (!r["password_otp_expires"].isNull()) passwordOtpExpires_ = std::make_shared<long long>(r["password_otp_expires"].as<long long>());
    if (!r["status"].isNull())               status_             = std::make_shared<std::string>(r["status"].as<std::string>());
    if (!r["picture"].isNull())              picture_            = std::make_shared<std::string>(r["picture"].as<std::string>());
    if (!r["blocked"].isNull())              blocked_            = std::make_shared<bool>(r["blocked"].as<bool>());
    if (!r["blocked_reason"].isNull())       blockedReason_      = std::make_shared<std::string>(r["blocked_reason"].as<std::string>());
    if (!r["timezone"].isNull())             timezone_           = std::make_shared<std::string>(r["timezone"].as<std::string>());
    if (!r["created_by"].isNull())           createdBy_          = std::make_shared<std::string>(r["created_by"].as<std::string>());
    if (!r["updated_by"].isNull())           updatedBy_          = std::make_shared<std::string>(r["updated_by"].as<std::string>());
}

Users::Users(const Json::Value &j) noexcept(false) {
    if (j.isMember("id"))           id_       = std::make_shared<std::string>(j["id"].asString());
    if (j.isMember("code"))         code_     = std::make_shared<std::string>(j["code"].asString());
    if (j.isMember("name"))         name_     = std::make_shared<std::string>(j["name"].asString());
    if (j.isMember("phone"))        phone_    = std::make_shared<std::string>(j["phone"].asString());
    if (j.isMember("email"))        email_    = std::make_shared<std::string>(j["email"].asString());
    if (j.isMember("password"))     password_ = std::make_shared<std::string>(j["password"].asString());
    if (j.isMember("status"))       status_   = std::make_shared<std::string>(j["status"].asString());
    if (j.isMember("timezone"))     timezone_ = std::make_shared<std::string>(j["timezone"].asString());
    if (j.isMember("created_by"))   createdBy_= std::make_shared<std::string>(j["created_by"].asString());
    if (j.isMember("updated_by"))   updatedBy_= std::make_shared<std::string>(j["updated_by"].asString());
}

const std::vector<std::string> &Users::insertColumns() noexcept { return insertColumns_; }

void Users::outputArgs(drogon::orm::internal::SqlBinder &binder) const {
    if (id_)                 binder << *id_;                 else binder << nullptr;
    if (code_)               binder << *code_;               else binder << nullptr;
    if (name_)               binder << *name_;               else binder << nullptr;
    if (phone_)              binder << *phone_;              else binder << nullptr;
    if (email_)              binder << *email_;              else binder << nullptr;
    if (password_)           binder << *password_;           else binder << nullptr;
    if (passwordOtp_)        binder << *passwordOtp_;        else binder << nullptr;
    if (passwordOtpExpires_) binder << *passwordOtpExpires_; else binder << nullptr;
    if (status_)             binder << *status_;             else binder << nullptr;
    if (picture_)            binder << *picture_;            else binder << nullptr;
    if (blocked_)            binder << *blocked_;            else binder << nullptr;
    if (blockedReason_)      binder << *blockedReason_;      else binder << nullptr;
    if (timezone_)           binder << *timezone_;           else binder << nullptr;
    if (createdBy_)          binder << *createdBy_;          else binder << nullptr;
    if (updatedBy_)          binder << *updatedBy_;          else binder << nullptr;
}

const std::vector<std::string> Users::updateColumns() const {
    std::vector<std::string> cols;
    if (code_)               cols.push_back("code");
    if (name_)               cols.push_back("name");
    if (phone_)              cols.push_back("phone");
    if (email_)              cols.push_back("email");
    if (password_)           cols.push_back("password");
    if (passwordOtp_)        cols.push_back("password_otp");
    if (passwordOtpExpires_) cols.push_back("password_otp_expires");
    if (status_)             cols.push_back("status");
    if (picture_)            cols.push_back("picture");
    if (blocked_)            cols.push_back("blocked");
    if (blockedReason_)      cols.push_back("blocked_reason");
    if (timezone_)           cols.push_back("timezone");
    if (updatedBy_)          cols.push_back("updated_by");
    return cols;
}

void Users::updateArgs(drogon::orm::internal::SqlBinder &binder) const {
    if (code_)               binder << *code_;
    if (name_)               binder << *name_;
    if (phone_)              binder << *phone_;
    if (email_)              binder << *email_;
    if (password_)           binder << *password_;
    if (passwordOtp_)        binder << *passwordOtp_;
    if (passwordOtpExpires_) binder << *passwordOtpExpires_;
    if (status_)             binder << *status_;
    if (picture_)            binder << *picture_;
    if (blocked_)            binder << *blocked_;
    if (blockedReason_)      binder << *blockedReason_;
    if (timezone_)           binder << *timezone_;
    if (updatedBy_)          binder << *updatedBy_;
}

Json::Value Users::toJson() const {
    Json::Value j;
    if (id_)       j["id"]       = *id_;
    if (code_)     j["code"]     = *code_;
    if (name_)     j["name"]     = *name_;
    if (phone_)    j["phone"]    = *phone_;
    if (email_)    j["email"]    = *email_;
    if (status_)   j["status"]   = *status_;
    if (picture_)  j["picture"]  = *picture_;
    if (blocked_)  j["blocked"]  = *blocked_;
    if (timezone_) j["timezone"] = *timezone_;
    return j;
}

std::string Users::sqlForFindingByPrimaryKey() {
    return "SELECT * FROM users WHERE id = ?";
}
std::string Users::sqlForDeletingByPrimaryKey() {
    return "DELETE FROM users WHERE id = ?";
}
std::string Users::sqlForInserting(bool &needSelection) const {
    needSelection = false;
    const auto &cols = insertColumns_;
    std::string sql = "INSERT INTO users (";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) sql += ","; sql += cols[i]; }
    sql += ") VALUES (";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) sql += ","; sql += "?"; }
    sql += ")";
    return sql;
}

}} // namespace
