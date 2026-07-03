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

class Settings {
  friend drogon::orm::Mapper<Settings>;
#ifdef __cpp_impl_coroutine
  friend drogon::orm::CoroMapper<Settings>;
#endif
public:
  static const constexpr char *tableName = "settings";
  static const bool hasPrimaryKey = true;
  static const std::string primaryKeyName;
  using PrimaryKeyType = std::string;
  const PrimaryKeyType &getPrimaryKey() const { return *id_; }

  struct Cols {
    static const std::string _id;
    static const std::string _name;
    static const std::string _theme;
    static const std::string _created_by;
    static const std::string _updated_by;
  };

  explicit Settings() = default;
  explicit Settings(const drogon::orm::Row &r, const ssize_t indexOffset = 0) noexcept;
  explicit Settings(const Json::Value &pJson) noexcept(false);

  const std::string &getValueOfId()         const noexcept { static std::string d; return id_         ? *id_         : d; }
  const std::string *getInitial()           const noexcept { return initial_.get(); }
  const std::string *getName()              const noexcept { return name_.get(); }
  const std::string *getDescription()       const noexcept { return description_.get(); }
  const std::string *getIcon()              const noexcept { return icon_.get(); }
  const std::string *getLogo()              const noexcept { return logo_.get(); }
  const std::string *getFavicon()           const noexcept { return favicon_.get(); }
  const std::string *getLoginImage()        const noexcept { return loginImage_.get(); }
  const std::string *getPhone()             const noexcept { return phone_.get(); }
  const std::string *getAddress()           const noexcept { return address_.get(); }
  const std::string *getEmail()             const noexcept { return email_.get(); }
  const std::string *getCopyright()         const noexcept { return copyright_.get(); }
  const std::string &getValueOfTheme()      const noexcept { static std::string d{"Blue"}; return theme_ ? *theme_ : d; }
  const std::string *getFeTemplate()        const noexcept { return feTemplate_.get(); }
  const std::string &getValueOfCreatedBy()  const noexcept { static std::string d; return createdBy_ ? *createdBy_ : d; }
  const std::string &getValueOfUpdatedBy()  const noexcept { static std::string d; return updatedBy_ ? *updatedBy_ : d; }

  void setId(const std::string &v)          { id_         = std::make_shared<std::string>(v); }
  void setInitial(const std::string &v)     { initial_    = std::make_shared<std::string>(v); }
  void setInitialToNull()                   { initial_.reset(); }
  void setName(const std::string &v)        { name_       = std::make_shared<std::string>(v); }
  void setNameToNull()                      { name_.reset(); }
  void setDescription(const std::string &v) { description_= std::make_shared<std::string>(v); }
  void setDescriptionToNull()               { description_.reset(); }
  void setIcon(const std::string &v)        { icon_       = std::make_shared<std::string>(v); }
  void setIconToNull()                      { icon_.reset(); }
  void setLogo(const std::string &v)        { logo_       = std::make_shared<std::string>(v); }
  void setLogoToNull()                      { logo_.reset(); }
  void setFavicon(const std::string &v)     { favicon_    = std::make_shared<std::string>(v); }
  void setFaviconToNull()                   { favicon_.reset(); }
  void setLoginImage(const std::string &v)  { loginImage_ = std::make_shared<std::string>(v); }
  void setLoginImageToNull()                { loginImage_.reset(); }
  void setPhone(const std::string &v)       { phone_      = std::make_shared<std::string>(v); }
  void setPhoneToNull()                     { phone_.reset(); }
  void setAddress(const std::string &v)     { address_    = std::make_shared<std::string>(v); }
  void setAddressToNull()                   { address_.reset(); }
  void setEmail(const std::string &v)       { email_      = std::make_shared<std::string>(v); }
  void setEmailToNull()                     { email_.reset(); }
  void setCopyright(const std::string &v)   { copyright_  = std::make_shared<std::string>(v); }
  void setCopyrightToNull()                 { copyright_.reset(); }
  void setTheme(const std::string &v)       { theme_      = std::make_shared<std::string>(v); }
  void setFeTemplate(const std::string &v)  { feTemplate_ = std::make_shared<std::string>(v); }
  void setFeTemplateToNull()                { feTemplate_.reset(); }
  void setCreatedBy(const std::string &v)   { createdBy_  = std::make_shared<std::string>(v); }
  void setUpdatedBy(const std::string &v)   { updatedBy_  = std::make_shared<std::string>(v); }

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
  std::shared_ptr<std::string> initial_;
  std::shared_ptr<std::string> name_;
  std::shared_ptr<std::string> description_;
  std::shared_ptr<std::string> icon_;
  std::shared_ptr<std::string> logo_;
  std::shared_ptr<std::string> favicon_;
  std::shared_ptr<std::string> loginImage_;
  std::shared_ptr<std::string> phone_;
  std::shared_ptr<std::string> address_;
  std::shared_ptr<std::string> email_;
  std::shared_ptr<std::string> copyright_;
  std::shared_ptr<std::string> theme_;
  std::shared_ptr<std::string> feTemplate_;
  std::shared_ptr<std::string> createdBy_;
  std::shared_ptr<std::string> updatedBy_;
  static const std::vector<std::string> insertColumns_;
};

}} // namespace
