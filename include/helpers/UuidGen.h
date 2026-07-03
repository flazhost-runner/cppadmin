#pragma once
#include <string>
#include <drogon/utils/Utilities.h>

// Generate a new UUID v4 string (e.g. "550e8400-e29b-41d4-a716-446655440000").
// Uses Drogon's built-in UUID generator (thread-safe).
inline std::string newUuid() {
    return drogon::utils::getUuid();
}
