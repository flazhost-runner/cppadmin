#include "bcrypt.h"
#include <openssl/rand.h>
#include <crypt.h>
#include <cstring>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace bcrypt {

static std::string makeSalt(int rounds) {
    // Generate 16 random bytes → base64-like salt for $2b$
    unsigned char raw[16];
    RAND_bytes(raw, 16);

    // crypt_gensalt_rn is not widely available; build a $2b$ salt manually.
    // crypt_r with $2b$rounds$<22-char-salt>
    static const char b64chars[] =
        "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string salt = "$2b$";
    if (rounds < 10) salt += "0";
    salt += std::to_string(rounds);
    salt += "$";
    // 22 base64 chars from 16 random bytes
    unsigned int val = 0;
    int bits = 0;
    for (int i = 0; i < 16 && salt.size() - salt.rfind('$') - 1 < 22; ++i) {
        val = (val << 8) | raw[i];
        bits += 8;
        while (bits >= 6) {
            bits -= 6;
            salt += b64chars[(val >> bits) & 0x3f];
        }
    }
    while ((int)(salt.size() - salt.rfind('$') - 1) < 22)
        salt += b64chars[0];
    return salt;
}

std::string hash(const std::string &password, int rounds) {
    std::string salt = makeSalt(rounds);
    struct crypt_data data{};
    data.initialized = 0;
    const char *result = crypt_r(password.c_str(), salt.c_str(), &data);
    if (!result || result[0] == '*')
        throw std::runtime_error("bcrypt::hash failed");
    return std::string(result);
}

bool verify(const std::string &password, const std::string &storedHash) {
    if (storedHash.size() < 7) return false;
    struct crypt_data data{};
    data.initialized = 0;
    const char *result = crypt_r(password.c_str(), storedHash.c_str(), &data);
    if (!result || result[0] == '*') return false;
    return storedHash == std::string(result);
}

} // namespace bcrypt
