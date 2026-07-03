#pragma once
#include <string>
#include <map>
#include <drogon/HttpRequest.h>
#include <json/json.h>

// Flash messages + field errors + old input — stored in session for one redirect (PRG pattern).
// Web controllers read flash after redirect and inject into HttpViewData via ViewHelper::injectFlash().

struct FlashData {
    std::string success;
    std::string error;
    std::string fieldErrorsJson; // JSON object string, e.g. {"email":"required"}
    std::string oldInputJson;    // JSON object string with previous form values
};

namespace Flash {

inline void setSuccess(const drogon::HttpRequestPtr &req, const std::string &msg) {
    req->session()->insert("flash_key",     std::string("success"));
    req->session()->insert("flash_message", msg);
}

inline void setError(const drogon::HttpRequestPtr &req, const std::string &msg) {
    req->session()->insert("flash_key",     std::string("error"));
    req->session()->insert("flash_message", msg);
}

// Store per-field validation errors + old input so forms can re-render inline errors.
inline void setFieldErrors(const drogon::HttpRequestPtr &req,
                            const std::map<std::string, std::string> &errors,
                            const std::map<std::string, std::string> &old) {
    Json::Value eJson(Json::objectValue);
    for (auto &[k, v] : errors) eJson[k] = v;
    Json::Value oJson(Json::objectValue);
    for (auto &[k, v] : old) oJson[k] = v;

    Json::FastWriter w;
    req->session()->insert("field_errors", w.write(eJson));
    req->session()->insert("old_input",    w.write(oJson));
}

// Consume flash from session — one-time read, clears keys.
// Returns FlashData struct; caller injects into HttpViewData via ViewHelper::injectFlash().
inline FlashData consume(const drogon::HttpRequestPtr &req) {
    FlashData f;
    auto fk  = req->session()->getOptional<std::string>("flash_key");
    auto fm  = req->session()->getOptional<std::string>("flash_message");
    auto fe  = req->session()->getOptional<std::string>("field_errors");
    auto old = req->session()->getOptional<std::string>("old_input");

    if (fk) {
        if (*fk == "success") f.success = fm ? *fm : "";
        else                   f.error   = fm ? *fm : "";
        req->session()->erase("flash_key");
        req->session()->erase("flash_message");
    }
    if (fe) {
        f.fieldErrorsJson = *fe;
        req->session()->erase("field_errors");
    }
    if (old) {
        f.oldInputJson = *old;
        req->session()->erase("old_input");
    }
    return f;
}

} // namespace Flash
