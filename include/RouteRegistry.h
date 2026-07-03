#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

// Named-route registry — required for RBAC route-driven (no named routes built-in to Drogon).
// Filled at bootstrap by each module's registerRoutes() call.
// RBAC filter reverse-looks up (method, pathPattern) → name → HasAccess.
// name AND method must both match — GET vs DELETE on same path = different permissions.

struct RouteEntry {
    std::string name;      // e.g. "admin.v1.access.user.delete"
    std::string method;    // "GET" | "POST" | "PUT" | "DELETE"
    std::string path;      // e.g. "/admin/v1/access/user/{id}/delete"
    std::string guardName; // "api" if name starts with "api.", else "web"
};

class RouteRegistry {
public:
    static RouteRegistry &instance() {
        static RouteRegistry reg;
        return reg;
    }

    // Register a named route. guardName inferred from name prefix.
    void add(const std::string &name, const std::string &method, const std::string &path) {
        std::lock_guard<std::mutex> lk(mu_);
        std::string guard = (name.rfind("api.", 0) == 0) ? "api" : "web";
        RouteEntry e{name, method, path, guard};
        byName_[name + "|" + method] = e;
        // Build reverse-lookup key: "METHOD|/path/pattern"
        byMethodPath_[method + "|" + path] = e;
        all_.push_back(e);
    }

    // Reverse lookup: given HTTP method + exact request path, return route name.
    // Supports simple {param} path patterns via prefix matching.
    std::string nameByMethodAndPath(const std::string &method,
                                     const std::string &reqPath) const {
        std::lock_guard<std::mutex> lk(mu_);
        // 1. Exact match
        auto it = byMethodPath_.find(method + "|" + reqPath);
        if (it != byMethodPath_.end()) return it->second.name;
        // 2. Pattern match: replace {param} segments
        for (auto &[key, entry] : byMethodPath_) {
            if (entry.method != method) continue;
            if (pathMatches(entry.path, reqPath)) return entry.name;
        }
        return "";
    }

    const std::vector<RouteEntry> &all() const { return all_; }

    RouteRegistry(const RouteRegistry &) = delete;
    RouteRegistry &operator=(const RouteRegistry &) = delete;

private:
    RouteRegistry() = default;

    // Returns true if pattern (with {param} segments) matches actual path.
    static bool pathMatches(const std::string &pattern, const std::string &actual) {
        // Split both by '/'
        auto split = [](const std::string &s) {
            std::vector<std::string> parts;
            std::string cur;
            for (char c : s) {
                if (c == '/') { parts.push_back(cur); cur.clear(); }
                else cur += c;
            }
            parts.push_back(cur);
            return parts;
        };
        auto pp = split(pattern);
        auto ap = split(actual);
        if (pp.size() != ap.size()) return false;
        for (size_t i = 0; i < pp.size(); ++i) {
            if (pp[i].empty() && ap[i].empty()) continue;
            if (!pp[i].empty() && pp[i].front() == '{') continue; // wildcard
            if (pp[i] != ap[i]) return false;
        }
        return true;
    }

    mutable std::mutex mu_;
    std::unordered_map<std::string, RouteEntry> byName_;
    std::unordered_map<std::string, RouteEntry> byMethodPath_;
    std::vector<RouteEntry> all_;
};

// Convenience macro — register a route and bind it to Drogon controller in one step.
// Usage: ROUTE(UserController, index, "GET", "/admin/v1/access/user", "admin.v1.access.user.index")
#define ROUTE_REG(name_, method_, path_) \
    RouteRegistry::instance().add((name_), (method_), (path_))
