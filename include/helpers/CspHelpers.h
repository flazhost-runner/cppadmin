#pragma once
#include <drogon/utils/OStringStream.h>
#include <drogon/HttpViewData.h>
#include "HtmlEscape.h"
#include <json/json.h>
#include <string>

inline std::string v(const drogon::HttpViewData &d, const std::string &key) {
    return d.get<std::string>(key);
}

inline void hout(drogon::OStringStream &s, const std::string &val) {
    s << h(val);
}

inline void flashAlerts(drogon::OStringStream &out, const drogon::HttpViewData &d) {
    auto success = v(d, "flashSuccess");
    auto error   = v(d, "flashError");
    if (!success.empty())
        out << "<div class=\"alert alert-success\">" << h(success) << "</div>\n";
    if (!error.empty())
        out << "<div class=\"alert alert-error\">" << h(error) << "</div>\n";
}

inline void csrfInput(drogon::OStringStream &out, const drogon::HttpViewData &d) {
    out << "<input type=\"hidden\" name=\"_csrf\" value=\"" << h(v(d, "csrfToken")) << "\">\n";
}

// Render errorMessages[] array from view data (2nd flash jalur — inline validation errors)
inline void errorMessagesAlert(drogon::OStringStream &out, const drogon::HttpViewData &d) {
    auto json = v(d, "errorMessages");
    if (json.empty()) return;
    Json::Value arr;
    Json::Reader reader;
    if (!reader.parse(json, arr) || !arr.isArray() || arr.empty()) return;
    out << "<div class=\"alert alert-danger\">";
    for (const auto &msg : arr) {
        if (msg.isString()) out << "<p class=\"mb-0\">" << h(msg.asString()) << "</p>";
    }
    out << "</div>\n";
}

// Check if the current user has a given role (reads userRolesJson from view data)
inline bool hasRole(const drogon::HttpViewData &d, const std::string &roleName) {
    auto json = v(d, "userRolesJson");
    if (json.empty()) return false;
    Json::Value arr;
    Json::Reader reader;
    if (!reader.parse(json, arr) || !arr.isArray()) return false;
    for (const auto &r : arr) {
        if (r.isString() && r.asString() == roleName) return true;
    }
    return false;
}

inline void navItem(drogon::OStringStream &out,
                    const std::string &href, const std::string &label,
                    const std::string &icon, const std::string &currentPath) {
    bool active = (currentPath.find(href) != std::string::npos);
    out << "<a href=\"" << href << "\" class=\"nav-item";
    if (active) out << " active";
    out << "\"><span class=\"icon\">" << icon << "</span> " << h(label) << "</a>\n";
}

inline void themeCssVars(drogon::OStringStream &out, const drogon::HttpViewData &d) {
    out << ":root {\n";
    out << "  --primary:   " << v(d, "themePrimary")   << ";\n";
    out << "  --secondary: " << v(d, "themeSecondary") << ";\n";
    out << "  --theme-light: " << v(d, "themeLight")   << ";\n";
    out << "  --theme-dark:  " << v(d, "themeDark")    << ";\n";
    out << "}\n";
}
