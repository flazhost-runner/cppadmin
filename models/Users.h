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

class Users {
  friend drogon::orm::Mapper<Users>;
#ifdef __cpp_impl_coroutine
  friend drogon::orm::CoroMapper<Users>;
#endif
public:
  static const constexpr char *tableName = "users";
  static const bool hasPrimaryKey = true;
  static const std::string primaryKeyName;
  using PrimaryKeyType = std::string;
  const PrimaryKeyType &getPrimaryKey() const { return *id_; }

  // Column name constants for use with drogon::orm::Criteria
  struct Cols {
    static const std::string _id;
    static const std::string _code;
    static const std::string _name;
    static const std::string _phone;
    static const std::string _email;
    static const std::string _password;
    static const std::string _status;
    static const std::string _timezone;
    static const std::string _blocked;
    static const std::string _picture;
    static const std::string _created_by;
    static const std::string _updated_by;
  };

  explicit Users() = default;
  explicit Users(const drogon::orm::Row &r, const ssize_t indexOffset = 0) noexcept;
  explicit Users(const Json::Value &pJson) noexcept(false);

  const std::string &getValueOfId()       const noexcept { static std::string d; return id_       ? *id_       : d; }
  const std::string &getValueOfCode()     const noexcept { static std::string d; return code_     ? *code_     : d; }
  const std::string &getValueOfName()     const noexcept { static std::string d; return name_     ? *name_     : d; }
  const std::string &getValueOfPhone()    const noexcept { static std::string d; return phone_    ? *phone_    : d; }
  const std::string &getValueOfEmail()    const noexcept { static std::string d; return email_    ? *email_    : d; }
  const std::string &getValueOfPassword() const noexcept { static std::string d; return password_ ? *password_ : d; }
  const std::string &getValueOfStatus()   const noexcept { static std::string d; return status_   ? *status_   : d; }
  const std::string &getValueOfTimezone() const noexcept { static std::string d; return timezone_ ? *timezone_ : d; }
  const std::string *getPasswordOtp()        const noexcept { return passwordOtp_.get(); }
  const long long   *getPasswordOtpExpires() const noexcept { return passwordOtpExpires_.get(); }
  const std::string *getPicture()            const noexcept { return picture_.get(); }
  const std::string *getBlockedReason()      const noexcept { return blockedReason_.get(); }
  bool               getValueOfBlocked()     const noexcept { return blocked_ ? *blocked_ : false; }
  const std::string &getValueOfCreatedBy()   const noexcept { static std::string d; return createdBy_ ? *createdBy_ : d; }
  const std::string &getValueOfUpdatedBy()   const noexcept { static std::string d; return updatedBy_ ? *updatedBy_ : d; }

  void setId(const std::string &v)              { id_       = std::make_shared<std::string>(v); }
  void setCode(const std::string &v)            { code_     = std::make_shared<std::string>(v); }
  void setName(const std::string &v)            { name_     = std::make_shared<std::string>(v); }
  void setPhone(const std::string &v)           { phone_    = std::make_shared<std::string>(v); }
  void setEmail(const std::string &v)           { email_    = std::make_shared<std::string>(v); }
  void setPassword(const std::string &v)        { password_ = std::make_shared<std::string>(v); }
  void setStatus(const std::string &v)          { status_   = std::make_shared<std::string>(v); }
  void setTimezone(const std::string &v)        { timezone_ = std::make_shared<std::string>(v); }
  void setPasswordOtp(const std::string &v)     { passwordOtp_        = std::make_shared<std::string>(v); }
  void setPasswordOtpExpires(long long v)       { passwordOtpExpires_ = std::make_shared<long long>(v); }
  void setPasswordOtpToNull()                   { passwordOtp_.reset(); }
  void setPasswordOtpExpiresToNull()            { passwordOtpExpires_.reset(); }
  void setPicture(const std::string &v)         { picture_      = std::make_shared<std::string>(v); }
  void setPictureToNull()                       { picture_.reset(); }
  void setBlocked(bool v)                       { blocked_      = std::make_shared<bool>(v); }
  void setBlockedReason(const std::string &v)   { blockedReason_= std::make_shared<std::string>(v); }
  void setCreatedBy(const std::string &v)       { createdBy_    = std::make_shared<std::string>(v); }
  void setUpdatedBy(const std::string &v)       { updatedBy_    = std::make_shared<std::string>(v); }

  static std::string sqlForFindingByPrimaryKey();
  static std::string sqlForDeletingByPrimaryKey();
  std::string sqlForInserting(bool &needSelection) const;
  void updateId(unsigned long long) { /* UUID pre-set, ignore SQLite rowid */ }

  static const std::vector<std::string> &insertColumns() noexcept;
  void outputArgs(drogon::orm::internal::SqlBinder &binder) const;
  const std::vector<std::string> updateColumns() const;
  void updateArgs(drogon::orm::internal::SqlBinder &binder) const;
  Json::Value toJson() const;

private:
  std::shared_ptr<std::string> id_;
  std::shared_ptr<std::string> code_;
  std::shared_ptr<std::string> name_;
  std::shared_ptr<std::string> phone_;
  std::shared_ptr<std::string> email_;
  std::shared_ptr<std::string> password_;
  std::shared_ptr<std::string> passwordOtp_;
  std::shared_ptr<long long>   passwordOtpExpires_;
  std::shared_ptr<std::string> status_;
  std::shared_ptr<std::string> picture_;
  std::shared_ptr<bool>        blocked_;
  std::shared_ptr<std::string> blockedReason_;
  std::shared_ptr<std::string> timezone_;
  std::shared_ptr<std::string> createdBy_;
  std::shared_ptr<std::string> updatedBy_;
  static const std::vector<std::string> insertColumns_;
};

}} // namespace
