#include "Roles.h"
#include "helpers/SqlIdent.h"
#include <drogon/orm/Field.h>

namespace drogon_model { namespace cppadmin {

const std::string Roles::primaryKeyName = "id";

const std::string Roles::Cols::_id         = "id";
const std::string Roles::Cols::_name       = "name";
const std::string Roles::Cols::_guard_name = "guard_name";
const std::string Roles::Cols::_status     = "status";
const std::string Roles::Cols::_desc       = "desc";
const std::string Roles::Cols::_created_by = "created_by";
const std::string Roles::Cols::_updated_by = "updated_by";

const std::vector<std::string> Roles::insertColumns_ = {
    "id","name","guard_name","status","desc","created_by","updated_by"
};

Roles::Roles(const drogon::orm::Row &r, const ssize_t /*indexOffset*/) noexcept {
    if (!r["id"].isNull())         id_        = std::make_shared<std::string>(r["id"].as<std::string>());
    if (!r["name"].isNull())       name_      = std::make_shared<std::string>(r["name"].as<std::string>());
    if (!r["guard_name"].isNull()) guardName_ = std::make_shared<std::string>(r["guard_name"].as<std::string>());
    if (!r["status"].isNull())     status_    = std::make_shared<std::string>(r["status"].as<std::string>());
    if (!r["desc"].isNull())       desc_      = std::make_shared<std::string>(r["desc"].as<std::string>());
    if (!r["created_by"].isNull()) createdBy_ = std::make_shared<std::string>(r["created_by"].as<std::string>());
    if (!r["updated_by"].isNull()) updatedBy_ = std::make_shared<std::string>(r["updated_by"].as<std::string>());
}

Roles::Roles(const Json::Value &j) noexcept(false) {
    if (j.isMember("id"))         id_        = std::make_shared<std::string>(j["id"].asString());
    if (j.isMember("name"))       name_      = std::make_shared<std::string>(j["name"].asString());
    if (j.isMember("guard_name")) guardName_ = std::make_shared<std::string>(j["guard_name"].asString());
    if (j.isMember("status"))     status_    = std::make_shared<std::string>(j["status"].asString());
    if (j.isMember("desc"))       desc_      = std::make_shared<std::string>(j["desc"].asString());
    if (j.isMember("created_by")) createdBy_ = std::make_shared<std::string>(j["created_by"].asString());
    if (j.isMember("updated_by")) updatedBy_ = std::make_shared<std::string>(j["updated_by"].asString());
}

const std::vector<std::string> &Roles::insertColumns() noexcept { return insertColumns_; }

void Roles::outputArgs(drogon::orm::internal::SqlBinder &binder) const {
    if (id_)        binder << *id_;        else binder << nullptr;
    if (name_)      binder << *name_;      else binder << nullptr;
    if (guardName_) binder << *guardName_; else binder << nullptr;
    if (status_)    binder << *status_;    else binder << nullptr;
    if (desc_)      binder << *desc_;      else binder << nullptr;
    if (createdBy_) binder << *createdBy_; else binder << nullptr;
    if (updatedBy_) binder << *updatedBy_; else binder << nullptr;
}

// Dikutip karena Mapper::update menyambungnya langsung ke `SET <col> = ?`, dan
// `desc` reserved word. Urutan harus cocok dengan updateArgs().
const std::vector<std::string> Roles::updateColumns() const {
    std::vector<std::string> cols;
    if (name_)      cols.push_back(helpers::quoteIdent("name"));
    if (guardName_) cols.push_back(helpers::quoteIdent("guard_name"));
    if (status_)    cols.push_back(helpers::quoteIdent("status"));
    if (desc_)      cols.push_back(helpers::quoteIdent("desc"));
    if (updatedBy_) cols.push_back(helpers::quoteIdent("updated_by"));
    return cols;
}

void Roles::updateArgs(drogon::orm::internal::SqlBinder &binder) const {
    if (name_)      binder << *name_;
    if (guardName_) binder << *guardName_;
    if (status_)    binder << *status_;
    if (desc_)      binder << *desc_;
    if (updatedBy_) binder << *updatedBy_;
}

Json::Value Roles::toJson() const {
    Json::Value j;
    if (id_)        j["id"]         = *id_;
    if (name_)      j["name"]       = *name_;
    if (guardName_) j["guard_name"] = *guardName_;
    if (status_)    j["status"]     = *status_;
    if (desc_)      j["desc"]       = *desc_;
    return j;
}

std::string Roles::sqlForFindingByPrimaryKey() {
    return "SELECT * FROM roles WHERE id = ?";
}
std::string Roles::sqlForDeletingByPrimaryKey() {
    return "DELETE FROM roles WHERE id = ?";
}
std::string Roles::sqlForInserting(bool &needSelection) const {
    needSelection = false;
    const auto cols = helpers::quoteIdents(insertColumns_);
    std::string sql = "INSERT INTO roles (";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) sql += ","; sql += cols[i]; }
    sql += ") VALUES (";
    for (size_t i = 0; i < cols.size(); ++i) { if (i > 0) sql += ","; sql += "?"; }
    sql += ")";
    return sql;
}

}} // namespace
