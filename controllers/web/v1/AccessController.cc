#include "AccessController.h"
#include "../../../include/helpers/ViewHelper.h"
#include "../../../include/helpers/FlashHelper.h"
#include "../../../include/helpers/Pagination.h"
#include "../../../include/RouteRegistry.h"
#include <drogon/drogon.h>
#include <set>

using drogon::HttpRequestPtr;
using drogon::HttpResponsePtr;
using drogon::HttpResponse;

// Drogon 1.8.7: getParameter() does NOT parse multipart text parts.
// These helpers parse the raw multipart body directly when needed.

// Extract boundary string from Content-Type header (returns "" if not multipart).
static std::string multipartBoundary(const std::string &ct) {
    auto bpos = ct.find("boundary=");
    if (bpos == std::string::npos) return {};
    std::string b = "--" + ct.substr(bpos + 9);
    while (!b.empty() && (b.back() == ' ' || b.back() == '"' || b.back() == '\r' || b.back() == '\n'))
        b.pop_back();
    return b;
}

// Get all values for a field name from a multipart body.
static std::vector<std::string> parseMultipartAll(const std::string &body,
                                                    const std::string &boundary,
                                                    const std::string &fieldName) {
    std::vector<std::string> result;
    std::string nameAttr = "name=\"" + fieldName + "\"";
    size_t pos = 0;
    while ((pos = body.find(boundary, pos)) != std::string::npos) {
        pos += boundary.size();
        auto headerEnd = body.find("\r\n\r\n", pos);
        if (headerEnd == std::string::npos) break;
        std::string hdrs = body.substr(pos, headerEnd - pos);
        if (hdrs.find(nameAttr) == std::string::npos) continue;
        size_t vStart = headerEnd + 4;
        size_t vEnd   = body.find("\r\n" + boundary, vStart);
        if (vEnd == std::string::npos) break;
        result.push_back(body.substr(vStart, vEnd - vStart));
    }
    return result;
}

// Get a single field value; for multipart falls back to getParameter for URL query params.
static std::string getParam(const HttpRequestPtr &req,
                             const std::string &key,
                             const std::string &body,
                             const std::string &boundary) {
    if (!boundary.empty()) {
        auto vals = parseMultipartAll(body, boundary, key);
        if (!vals.empty()) return vals[0];
        // Also check URL query via Drogon for multipart (works for query string)
        return req->getParameter(key);
    }
    return req->getParameter(key);
}

// Parse multi-value params for both URL-encoded and multipart/form-data.
static std::vector<std::string> getMultiParam(const HttpRequestPtr &req,
                                               const std::string &rawKey) {
    std::vector<std::string> result;
    auto ct = req->getHeader("content-type");

    if (ct.find("multipart/form-data") != std::string::npos) {
        std::string bnd = multipartBoundary(ct);
        if (bnd.empty()) return result;
        return parseMultipartAll(std::string(req->getBody()), bnd, rawKey);
    }

    // URL-encoded: scan raw body for "key[]=" and "key%5B%5D=" forms
    std::string base = rawKey.substr(0, rawKey.size() - 2); // strip "[]"
    const std::string body(req->getBody());
    for (const std::string &prefix : {rawKey + "=", base + "%5B%5D="}) {
        size_t pos = 0;
        while ((pos = body.find(prefix, pos)) != std::string::npos) {
            bool atBound = (pos == 0 || body[pos - 1] == '&');
            pos += prefix.size();
            if (!atBound) continue;
            size_t end = body.find('&', pos);
            std::string val = (end == std::string::npos)
                ? body.substr(pos) : body.substr(pos, end - pos);
            if (!val.empty()) result.push_back(val);
        }
    }
    return result;
}

std::string AccessController::actorId(const HttpRequestPtr &req) const {
    auto attrs = req->getAttributes();
    return attrs->find("currentUser") ? attrs->get<std::string>("currentUser") : "system";
}

// ── Users ─────────────────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessController::usersIndex(HttpRequestPtr req) {
    int page     = std::max(1, atoi(req->getParameter("q_page").c_str()));
    int pageSize = std::max(10, atoi(req->getParameter("q_page_size").c_str()));
    std::string qCode   = req->getParameter("q_code");
    std::string qName   = req->getParameter("q_name");
    std::string qPhone  = req->getParameter("q_phone");
    std::string qEmail  = req->getParameter("q_email");
    std::string qStatus = req->getParameter("q_status");
    std::string qRole   = req->getParameter("q_role");

    auto result = co_await userSvc_->list(page, pageSize, qName, qEmail, qStatus, qCode, qPhone);
    auto meta   = makePaginateMeta(result.total, page, pageSize);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Users"));
    data.insert("activeMenu", std::string("access.users"));
    data.insert("qCode",      qCode);
    data.insert("qName",      qName);
    data.insert("qPhone",     qPhone);
    data.insert("qEmail",     qEmail);
    data.insert("qStatus",    qStatus);
    data.insert("qRole",      qRole);
    data.insert("currentPage", std::to_string(page));
    data.insert("totalPage",   std::to_string(meta.totalPages));
    data.insert("pageSize",    std::to_string(pageSize));

    // rows format: idx|id|code|name|phone|email|status|picture|roles(comma-sep)
    std::string rows;
    int idx = (page - 1) * pageSize + 1;
    for (const auto &u : result.rows) {
        std::string code  = u.getValueOfCode();
        std::string phone = u.getValueOfPhone();
        std::string pic   = u.getPicture() ? *u.getPicture() : "";
        // Fetch roles for this user
        std::string roleNames;
        try {
            auto urs = co_await userSvc_->rolesOf(u.getValueOfId());
            for (const auto &r : urs) {
                if (!roleNames.empty()) roleNames += ",";
                roleNames += r.getValueOfName();
            }
        } catch (...) {}
        rows += std::to_string(idx++) + "|"
             + u.getValueOfId()     + "|"
             + code + "|"
             + u.getValueOfName()   + "|"
             + phone + "|"
             + u.getValueOfEmail()  + "|"
             + u.getValueOfStatus() + "|"
             + pic + "|"
             + roleNames + "\n";
    }
    data.insert("rows", rows);

    co_return renderView("views::be::admin::access::users::index", data);
}

drogon::Task<HttpResponsePtr> AccessController::usersCreate(HttpRequestPtr req) {
    auto allRoles = co_await roleSvc_->list(1, 200, "", "");

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Add User"));
    data.insert("activeMenu", std::string("access.users"));

    std::string rolesHtml;
    for (const auto &r : allRoles.rows) {
        std::string id = r.getValueOfId(), name = r.getValueOfName();
        rolesHtml += "<div class=\"form-check form-check-inline\">"
                     "<input class=\"form-check-input\" type=\"checkbox\""
                     " name=\"roles[]\" value=\"" + id + "\" id=\"role_" + id + "\">"
                     "<label class=\"form-check-label\" for=\"role_" + id + "\">" + name + "</label>"
                     "</div>";
    }
    data.insert("rolesHtml", rolesHtml);

    co_return renderView("views::be::admin::access::users::create", data);
}

drogon::Task<HttpResponsePtr> AccessController::usersStore(HttpRequestPtr req) {
    auto ct   = req->getHeader("content-type");
    auto body = std::string(req->getBody());
    auto bnd  = multipartBoundary(ct);
    UserCreateInput input;
    input.name          = getParam(req, "name",           body, bnd);
    input.code          = getParam(req, "code",           body, bnd);
    input.email         = getParam(req, "email",          body, bnd);
    input.phone         = getParam(req, "phone",          body, bnd);
    input.password      = getParam(req, "password",       body, bnd);
    input.status        = getParam(req, "status",         body, bnd);
    input.timezone      = getParam(req, "timezone",       body, bnd);
    input.blocked       = getParam(req, "blocked",        body, bnd) == "1";
    input.blockedReason = getParam(req, "blocked_reason", body, bnd);

    for (const auto &rid : getMultiParam(req, "roles[]")) input.roleIds.push_back(rid);

    co_await userSvc_->create(input, actorId(req));
    Flash::setSuccess(req, "Create User Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/users");
}

drogon::Task<HttpResponsePtr> AccessController::usersShow(HttpRequestPtr req, std::string id) {
    auto user  = co_await userSvc_->findById(id);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Detail User"));
    data.insert("activeMenu", std::string("access.users"));
    data.insert("userId",     user.getValueOfId());
    data.insert("userName",   user.getValueOfName());
    data.insert("userEmail",  user.getValueOfEmail());
    data.insert("userPhone",  user.getValueOfPhone());
    data.insert("userCode",   user.getValueOfCode());
    data.insert("userStatus", user.getValueOfStatus());

    co_return renderView("views::be::admin::access::users::show", data);
}

drogon::Task<HttpResponsePtr> AccessController::usersEdit(HttpRequestPtr req, std::string id) {
    auto user      = co_await userSvc_->findById(id);
    auto userRoles = co_await userSvc_->rolesOf(id);
    auto allRoles  = co_await roleSvc_->list(1, 200, "", "");

    std::set<std::string> assignedIds;
    for (const auto &r : userRoles) assignedIds.insert(r.getValueOfId());

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",   std::string("Edit User"));
    data.insert("activeMenu",  std::string("access.users"));
    data.insert("userId",      user.getValueOfId());
    data.insert("userCode",    user.getValueOfCode());
    data.insert("userName",    user.getValueOfName());
    data.insert("userEmail",   user.getValueOfEmail());
    data.insert("userPhone",   user.getValueOfPhone());
    data.insert("userStatus",        user.getValueOfStatus());
    data.insert("userPicture",       user.getPicture()        ? *user.getPicture()        : std::string{});
    data.insert("userTimezone",      user.getValueOfTimezone().empty() ? std::string{"UTC"} : user.getValueOfTimezone());
    data.insert("userBlocked",       user.getValueOfBlocked() ? std::string{"1"}          : std::string{"0"});
    data.insert("userBlockedReason", user.getBlockedReason()  ? *user.getBlockedReason()  : std::string{});

    std::string rolesHtml;
    for (const auto &r : allRoles.rows) {
        std::string rid  = r.getValueOfId();
        std::string name = r.getValueOfName();
        bool sel = assignedIds.count(rid) > 0;
        rolesHtml += "<div class=\"form-check form-check-inline\">"
                     "<input class=\"form-check-input\" type=\"checkbox\""
                     " name=\"roles[]\" value=\"" + rid + "\" id=\"role_" + rid + "\""
                   + (sel ? " checked" : "") + ">"
                     "<label class=\"form-check-label\" for=\"role_" + rid + "\">" + name + "</label>"
                     "</div>";
    }
    data.insert("rolesHtml", rolesHtml);

    co_return renderView("views::be::admin::access::users::edit", data);
}

drogon::Task<HttpResponsePtr> AccessController::usersUpdate(HttpRequestPtr req, std::string id) {
    auto ct   = req->getHeader("content-type");
    auto body = std::string(req->getBody());
    auto bnd  = multipartBoundary(ct);
    UserUpdateInput input;
    input.name          = getParam(req, "name",           body, bnd);
    input.phone         = getParam(req, "phone",          body, bnd);
    input.status        = getParam(req, "status",         body, bnd);
    input.timezone      = getParam(req, "timezone",       body, bnd);
    input.blocked       = getParam(req, "blocked",        body, bnd) == "1";
    input.blockedReason = getParam(req, "blocked_reason", body, bnd);

    for (const auto &rid : getMultiParam(req, "roles[]")) input.roleIds.push_back(rid);

    co_await userSvc_->update(id, input, actorId(req));
    Flash::setSuccess(req, "Update User Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/users");
}

drogon::Task<HttpResponsePtr> AccessController::usersDestroy(HttpRequestPtr req, std::string id) {
    co_await userSvc_->remove(id);
    Flash::setSuccess(req, "Delete User Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/users");
}

drogon::Task<HttpResponsePtr> AccessController::usersDeleteSelected(HttpRequestPtr req) {
    auto selected = getMultiParam(req, "selected[]");
    for (const auto &id : selected) co_await userSvc_->remove(id);
    Flash::setSuccess(req, "Delete Users Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/users");
}

// ── Roles ─────────────────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessController::rolesIndex(HttpRequestPtr req) {
    int page     = std::max(1, atoi(req->getParameter("q_page").c_str()));
    int pageSize = std::max(10, atoi(req->getParameter("q_page_size").c_str()));
    std::string qName   = req->getParameter("q_name");
    std::string qStatus = req->getParameter("q_status");
    std::string qDesc   = req->getParameter("q_desc");

    auto result = co_await roleSvc_->list(page, pageSize, qName, qStatus);
    auto meta   = makePaginateMeta(result.total, page, pageSize);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Roles"));
    data.insert("activeMenu", std::string("access.roles"));
    data.insert("qName",      qName);
    data.insert("qStatus",    qStatus);
    data.insert("qDesc",      qDesc);
    data.insert("currentPage", std::to_string(page));
    data.insert("totalPage",   std::to_string(meta.totalPages));
    data.insert("pageSize",    std::to_string(pageSize));

    std::string rows;
    int idx = (page - 1) * pageSize + 1;
    for (const auto &r : result.rows) {
        std::string desc = r.getDesc() ? *r.getDesc() : "";
        rows += std::to_string(idx++) + "|"
             + r.getValueOfId()        + "|"
             + r.getValueOfName()      + "|"
             + r.getValueOfStatus()    + "|"
             + desc + "\n";
    }
    data.insert("rows", rows);

    co_return renderView("views::be::admin::access::roles::index", data);
}

drogon::Task<HttpResponsePtr> AccessController::rolesCreate(HttpRequestPtr req) {
    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Add Role"));
    data.insert("activeMenu", std::string("access.roles"));

    co_return renderView("views::be::admin::access::roles::create", data);
}

drogon::Task<HttpResponsePtr> AccessController::rolesStore(HttpRequestPtr req) {
    auto ct = req->getHeader("content-type"); auto body = std::string(req->getBody()); auto bnd = multipartBoundary(ct);
    RoleCreateInput input;
    input.name   = getParam(req, "name",   body, bnd);
    input.status = getParam(req, "status", body, bnd);
    input.desc   = getParam(req, "desc",   body, bnd);

    for (const auto &pid : getMultiParam(req, "permissionIds[]")) input.permissionIds.push_back(pid);

    co_await roleSvc_->create(input, actorId(req));
    Flash::setSuccess(req, "Create Role Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles");
}

drogon::Task<HttpResponsePtr> AccessController::rolesShow(HttpRequestPtr req, std::string id) {
    auto role = co_await roleSvc_->findById(id);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Detail Role"));
    data.insert("activeMenu", std::string("access.roles"));
    data.insert("roleId",     role.getValueOfId());
    data.insert("roleName",   role.getValueOfName());
    data.insert("roleStatus", role.getValueOfStatus());
    data.insert("roleDesc",   role.getDesc() ? *role.getDesc() : std::string{});

    co_return renderView("views::be::admin::access::roles::show", data);
}

drogon::Task<HttpResponsePtr> AccessController::rolesEdit(HttpRequestPtr req, std::string id) {
    auto role = co_await roleSvc_->findById(id);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Edit Role"));
    data.insert("activeMenu", std::string("access.roles"));
    data.insert("roleId",     role.getValueOfId());
    data.insert("roleName",   role.getValueOfName());
    data.insert("roleStatus", role.getValueOfStatus());
    data.insert("roleDesc",   role.getDesc() ? *role.getDesc() : std::string{});

    co_return renderView("views::be::admin::access::roles::edit", data);
}

drogon::Task<HttpResponsePtr> AccessController::rolesUpdate(HttpRequestPtr req, std::string id) {
    auto ct = req->getHeader("content-type"); auto body = std::string(req->getBody()); auto bnd = multipartBoundary(ct);
    RoleUpdateInput input;
    input.name   = getParam(req, "name",   body, bnd);
    input.status = getParam(req, "status", body, bnd);
    input.desc   = getParam(req, "desc",   body, bnd);

    for (const auto &pid : getMultiParam(req, "permissionIds[]")) input.permissionIds.push_back(pid);

    co_await roleSvc_->update(id, input, actorId(req));
    Flash::setSuccess(req, "Update Role Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles");
}

drogon::Task<HttpResponsePtr> AccessController::rolesDestroy(HttpRequestPtr req, std::string id) {
    co_await roleSvc_->remove(id);
    Flash::setSuccess(req, "Delete Role Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles");
}

drogon::Task<HttpResponsePtr> AccessController::rolesDeleteSelected(HttpRequestPtr req) {
    auto selected = getMultiParam(req, "selected[]");
    for (const auto &id : selected) co_await roleSvc_->remove(id);
    Flash::setSuccess(req, "Delete Roles Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles");
}

// ── Permissions ───────────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessController::permissionsIndex(HttpRequestPtr req) {
    int page     = std::max(1, atoi(req->getParameter("q_page").c_str()));
    int pageSize = std::max(10, atoi(req->getParameter("q_page_size").c_str()));
    std::string qName   = req->getParameter("q_name");
    std::string qGuard  = req->getParameter("q_guard");
    std::string qMethod = req->getParameter("q_method");
    std::string qStatus = req->getParameter("q_status");

    // Auto-discover: sync RouteRegistry entries → permissions table on first load
    {
        auto existing = co_await permSvc_->listAll();
        std::set<std::string> existingNames;
        for (const auto &p : existing) existingNames.insert(p.getValueOfName());

        const auto &routes = RouteRegistry::instance().all();
        for (const auto &route : routes) {
            if (existingNames.count(route.name)) continue;
            PermissionCreateInput pi;
            pi.name      = route.name;
            pi.guardName = route.guardName;
            pi.method    = route.method;
            pi.status    = "Active";
            try { co_await permSvc_->create(pi, "system"); }
            catch (...) {} // ignore duplicates
        }
    }

    auto result = co_await permSvc_->list(page, pageSize, qName, qGuard, qMethod, qStatus);
    auto meta   = makePaginateMeta(result.total, page, pageSize);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Permissions"));
    data.insert("activeMenu", std::string("access.permissions"));
    data.insert("qName",      qName);
    data.insert("qGuard",     qGuard);
    data.insert("qMethod",    qMethod);
    data.insert("qStatus",    qStatus);
    data.insert("currentPage", std::to_string(page));
    data.insert("totalPage",   std::to_string(meta.totalPages));
    data.insert("pageSize",    std::to_string(pageSize));

    std::string rows;
    int idx = (page - 1) * pageSize + 1;
    for (const auto &p : result.rows) {
        std::string desc = p.getDesc() ? *p.getDesc() : "";
        rows += std::to_string(idx++) + "|"
             + p.getValueOfId()        + "|"
             + p.getValueOfName()      + "|"
             + p.getValueOfGuardName() + "|"
             + p.getValueOfMethod()    + "|"
             + p.getValueOfStatus()    + "|"
             + desc + "\n";
    }
    data.insert("rows", rows);

    co_return renderView("views::be::admin::access::permissions::index", data);
}

drogon::Task<HttpResponsePtr> AccessController::permissionsCreate(HttpRequestPtr req) {
    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Add Permission"));
    data.insert("activeMenu", std::string("access.permissions"));

    co_return renderView("views::be::admin::access::permissions::create", data);
}

drogon::Task<HttpResponsePtr> AccessController::permissionsStore(HttpRequestPtr req) {
    auto ct = req->getHeader("content-type"); auto body = std::string(req->getBody()); auto bnd = multipartBoundary(ct);
    PermissionCreateInput input;
    input.name      = getParam(req, "name",       body, bnd);
    input.guardName = getParam(req, "guard_name", body, bnd);
    input.method    = getParam(req, "method",     body, bnd);
    input.status    = getParam(req, "status",     body, bnd);
    input.desc      = getParam(req, "desc",       body, bnd);

    co_await permSvc_->create(input, actorId(req));
    Flash::setSuccess(req, "Create Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/permissions");
}

drogon::Task<HttpResponsePtr> AccessController::permissionsShow(HttpRequestPtr req, std::string id) {
    auto perm = co_await permSvc_->findById(id);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Detail Permission"));
    data.insert("activeMenu", std::string("access.permissions"));
    data.insert("permId",     perm.getValueOfId());
    data.insert("permName",   perm.getValueOfName());
    data.insert("permGuard",  perm.getValueOfGuardName());
    data.insert("permMethod", perm.getValueOfMethod());
    data.insert("permStatus", perm.getValueOfStatus());
    data.insert("permDesc",   perm.getDesc() ? *perm.getDesc() : std::string{});

    co_return renderView("views::be::admin::access::permissions::show", data);
}

drogon::Task<HttpResponsePtr> AccessController::permissionsEdit(HttpRequestPtr req, std::string id) {
    auto perm = co_await permSvc_->findById(id);

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Edit Permission"));
    data.insert("activeMenu", std::string("access.permissions"));
    data.insert("permId",     perm.getValueOfId());
    data.insert("permName",   perm.getValueOfName());
    data.insert("permGuard",  perm.getValueOfGuardName());
    data.insert("permMethod", perm.getValueOfMethod());
    data.insert("permStatus", perm.getValueOfStatus());
    data.insert("permDesc",   perm.getDesc() ? *perm.getDesc() : std::string{});

    co_return renderView("views::be::admin::access::permissions::edit", data);
}

drogon::Task<HttpResponsePtr> AccessController::permissionsUpdate(HttpRequestPtr req, std::string id) {
    auto ct = req->getHeader("content-type"); auto body = std::string(req->getBody()); auto bnd = multipartBoundary(ct);
    PermissionUpdateInput input;
    input.name      = getParam(req, "name",       body, bnd);
    input.guardName = getParam(req, "guard_name", body, bnd);
    input.method    = getParam(req, "method",     body, bnd);
    input.status    = getParam(req, "status",     body, bnd);
    input.desc      = getParam(req, "desc",       body, bnd);

    co_await permSvc_->update(id, input, actorId(req));
    Flash::setSuccess(req, "Update Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/permissions");
}

drogon::Task<HttpResponsePtr> AccessController::permissionsDestroy(HttpRequestPtr req, std::string id) {
    co_await permSvc_->remove(id);
    Flash::setSuccess(req, "Delete Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/permissions");
}

drogon::Task<HttpResponsePtr> AccessController::permissionsDeleteSelected(HttpRequestPtr req) {
    auto selected = getMultiParam(req, "selected[]");
    for (const auto &id : selected) co_await permSvc_->remove(id);
    Flash::setSuccess(req, "Delete Permissions Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/permissions");
}

// ── Role → Permission ─────────────────────────────────────────────────────────

drogon::Task<HttpResponsePtr> AccessController::rolesPermission(HttpRequestPtr req, std::string id) {
    int page     = std::max(1, atoi(req->getParameter("q_page").c_str()));
    int pageSize = std::max(10, atoi(req->getParameter("q_page_size").c_str()));
    std::string qName   = req->getParameter("q_name");
    std::string qStatus = req->getParameter("q_status");

    auto role     = co_await roleSvc_->findById(id);
    auto assigned = co_await roleSvc_->permissionsOf(id);
    auto result   = co_await permSvc_->list(page, pageSize, qName, "", "", qStatus);
    auto meta     = makePaginateMeta(result.total, page, pageSize);

    std::set<std::string> assignedIds;
    for (const auto &p : assigned) assignedIds.insert(p.getValueOfId());

    std::string rows;
    int idx = (page - 1) * pageSize + 1;
    for (const auto &p : result.rows) {
        bool isAssigned = assignedIds.count(p.getValueOfId()) > 0;
        std::string desc = p.getDesc() ? *p.getDesc() : "";
        rows += std::to_string(idx++) + "|"
             + p.getValueOfId()     + "|"
             + p.getValueOfName()   + "|"
             + p.getValueOfStatus() + "|"
             + desc + "|"
             + (isAssigned ? "1" : "0") + "\n";
    }

    drogon::HttpViewData data;
    prepareViewData(data, req);
    data.insert("pageTitle",  std::string("Permission Management"));
    data.insert("activeMenu", std::string("access.roles"));
    data.insert("roleId",     id);
    data.insert("roleName",   role.getValueOfName());
    data.insert("rows",       rows);
    data.insert("qName",      qName);
    data.insert("qStatus",    qStatus);
    data.insert("currentPage", std::to_string(page));
    data.insert("totalPage",   std::to_string(meta.totalPages));
    data.insert("pageSize",    std::to_string(pageSize));
    co_return renderView("views::be::admin::access::roles::permission", data);
}

drogon::Task<HttpResponsePtr> AccessController::rolesPermissionAssign(HttpRequestPtr req, std::string id, std::string permId) {
    co_await roleSvc_->assignPermission(id, permId);
    Flash::setSuccess(req, "Assign Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles/" + id + "/permission");
}

drogon::Task<HttpResponsePtr> AccessController::rolesPermissionUnassign(HttpRequestPtr req, std::string id, std::string permId) {
    co_await roleSvc_->unassignPermission(id, permId);
    Flash::setSuccess(req, "Unassign Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles/" + id + "/permission");
}

drogon::Task<HttpResponsePtr> AccessController::rolesPermissionAssignSelected(HttpRequestPtr req, std::string id) {
    auto selected = getMultiParam(req, "selected[]");
    co_await roleSvc_->assignPermissions(id, selected);
    Flash::setSuccess(req, "Assign Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles/" + id + "/permission");
}

drogon::Task<HttpResponsePtr> AccessController::rolesPermissionUnassignSelected(HttpRequestPtr req, std::string id) {
    auto selected = getMultiParam(req, "selected[]");
    co_await roleSvc_->unassignPermissions(id, selected);
    Flash::setSuccess(req, "Unassign Permission Success.");
    co_return HttpResponse::newRedirectionResponse("/admin/v1/access/roles/" + id + "/permission");
}
