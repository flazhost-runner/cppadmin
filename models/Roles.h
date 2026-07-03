#pragma once
#include <drogon/orm/Field.h>
#include <drogon/orm/Mapper.h>
#include <drogon/orm/CoroMapper.h>
#include <drogon/orm/SqlBinder.h>
#include <string>
#include <memory>
#include <vector>
#include <json/json.h>

namespace drogon_model { namespace cppadmin {

class Roles {
  friend drogon::orm::Mapper<Roles>;
#ifdef __cpp_impl_coroutine
  friend drogon::orm::CoroMapper<Roles>;
#endif
public:
  static const constexpr char *tableName = "roles";
  static const bool hasPrimaryKey = true;
  static const std::string primaryKeyName;
  using PrimaryKeyType = std::string;
  const PrimaryKeyType &getPrimaryKey() const { return *id_; }

  struct Cols {
    static const std::string _id;
    static const std::string _name;
    static const std::string _guard_name;
    static const std::string _status;
    static const std::string _desc;
    static const std::string _created_by;
    static const std::string _updated_by;
  };

  explicit Roles() = default;
  explicit Roles(const drogon::orm::Row &r, const ssize_t indexOffset = 0) noexcept;
  explicit Roles(const Json::Value &pJson) noexcept(false);

  const std::string &getValueOfId()         const noexcept { static std::string d; return id_        ? *id_        : d; }
  const std::string &getValueOfName()       const noexcept { static std::string d; return name_      ? *name_      : d; }
  const std::string &getValueOfGuardName()  const noexcept { static std::string d; return guardName_ ? *guardName_ : d; }
  const std::string &getValueOfStatus()     const noexcept { static std::string d; return status_    ? *status_    : d; }
  const std::string *getDesc()              const noexcept { return desc_.get(); }
  const std::string &getValueOfCreatedBy()  const noexcept { static std::string d; return createdBy_ ? *createdBy_ : d; }
  const std::string &getValueOfUpdatedBy()  const noexcept { static std::string d; return updatedBy_ ? *updatedBy_ : d; }

  void setId(const std::string &v)         { id_        = std::make_shared<std::string>(v); }
  void setName(const std::string &v)       { name_      = std::make_shared<std::string>(v); }
  void setGuardName(const std::string &v)  { guardName_ = std::make_shared<std::string>(v); }
  void setStatus(const std::string &v)     { status_    = std::make_shared<std::string>(v); }
  void setDesc(const std::string &v)       { desc_      = std::make_shared<std::string>(v); }
  void setDescToNull()                     { desc_.reset(); }
  void setCreatedBy(const std::string &v)  { createdBy_ = std::make_shared<std::string>(v); }
  void setUpdatedBy(const std::string &v)  { updatedBy_ = std::make_shared<std::string>(v); }

  static std::string sqlForFindingByPrimaryKey();
  static std::string sqlForDeletingByPrimaryKey();
  std::string sqlForInserting(bool &needSelection) const;
  void updateId(unsigned long long) {}

  static const std::vector<std::string> &insertColumns() noexcept;
  void outputArgs(drogon::orm::internal::SqlBinder &binder) const;
  const std::vector<std::string> updateColumns() const;
  void updateArgs(drogon::orm::internal::SqlBinder &binder) const;
  Json::Value toJson() const;

private:
  std::shared_ptr<std::string> id_;
  std::shared_ptr<std::string> name_;
  std::shared_ptr<std::string> guardName_;
  std::shared_ptr<std::string> status_;
  std::shared_ptr<std::string> desc_;
  std::shared_ptr<std::string> createdBy_;
  std::shared_ptr<std::string> updatedBy_;
  static const std::vector<std::string> insertColumns_;
};

}} // namespace
