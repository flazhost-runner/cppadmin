#pragma once
#include <string>

// Thin bcrypt wrapper using OpenSSL's crypt()-compatible EVP + crypt_r.
// On Ubuntu 24.04 glibc/libxcrypt supports $2b$ (bcrypt) natively via crypt_r().

namespace bcrypt {

// Hash a plaintext password with bcrypt (rounds from AppConfig::bcryptRounds).
// Returns the full hash string: "$2b$10$..."
std::string hash(const std::string &password, int rounds = 10);

// Verify a plaintext password against a stored bcrypt hash.
// Returns true if they match.
bool verify(const std::string &password, const std::string &hash);

} // namespace bcrypt
