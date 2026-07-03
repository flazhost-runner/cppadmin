#pragma once
#include <string>

// Case-insensitive LIKE helper — cross-dialect (MySQL/PG/SQLite).
// Returns the SQL fragment and value to use with Drogon Criteria or raw SQL.
// Usage: auto [sql, val] = ciLike("u.name", q_name);
//        criteria.andCondition(sql, val);

inline std::pair<std::string, std::string> ciLike(
    const std::string &col, const std::string &val)
{
    // LOWER(col) LIKE LOWER('%val%')
    std::string sql  = "LOWER(" + col + ") LIKE LOWER(?)";
    std::string bind = "%" + val + "%";
    return {sql, bind};
}

// Removes a key prefix from a query-param name.
// e.g. removePrefix("q_name", "q_") → "name"
inline std::string removePrefix(const std::string &s, const std::string &prefix) {
    if (s.rfind(prefix, 0) == 0) return s.substr(prefix.size());
    return s;
}
