#pragma once
// Minimal JWT HS256 implementation using only OpenSSL.
// Replaces libjwt — no extra package needed.
// Only implements what CppAdmin needs: sign + verify HS256 tokens.
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <string>
#include <sstream>
#include <vector>
#include <json/json.h>
#include <chrono>
#include <stdexcept>

namespace jwt_helper {

// RFC 4648 base64url (no padding)
inline std::string base64UrlEncode(const unsigned char *data, size_t len) {
    static const char *enc =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned int b = (data[i] << 16);
        if (i + 1 < len) b |= (data[i+1] << 8);
        if (i + 2 < len) b |= data[i+2];
        out += enc[(b >> 18) & 0x3f];
        out += enc[(b >> 12) & 0x3f];
        out += (i + 1 < len) ? enc[(b >> 6) & 0x3f] : '=';
        out += (i + 2 < len) ? enc[b & 0x3f]         : '=';
    }
    // Convert to url-safe, strip padding
    for (char &c : out) {
        if (c == '+') c = '-';
        else if (c == '/') c = '_';
    }
    while (!out.empty() && out.back() == '=') out.pop_back();
    return out;
}

inline std::string base64UrlEncode(const std::string &s) {
    return base64UrlEncode(reinterpret_cast<const unsigned char *>(s.data()), s.size());
}

// Decode base64url → bytes (returns empty on error)
inline std::vector<unsigned char> base64UrlDecode(const std::string &in) {
    std::string s = in;
    for (char &c : s) {
        if (c == '-') c = '+';
        else if (c == '_') c = '/';
    }
    while (s.size() % 4) s += '=';

    std::vector<unsigned char> out;
    out.resize(s.size() * 3 / 4);
    int len = EVP_DecodeBlock(out.data(),
                              reinterpret_cast<const unsigned char *>(s.data()),
                              (int)s.size());
    if (len < 0) return {};
    // Strip padding bytes
    int pad = 0;
    if (!s.empty() && s[s.size()-1] == '=') pad++;
    if (s.size() > 1 && s[s.size()-2] == '=') pad++;
    out.resize(len - pad);
    return out;
}

// HMAC-SHA256
inline std::string hmacSha256(const std::string &key, const std::string &data) {
    unsigned char digest[EVP_MAX_MD_SIZE];
    unsigned int dlen = 0;
    HMAC(EVP_sha256(),
         key.data(), (int)key.size(),
         reinterpret_cast<const unsigned char *>(data.data()), data.size(),
         digest, &dlen);
    return std::string(reinterpret_cast<char *>(digest), dlen);
}

// Sign a JWT with HS256 — returns token string
inline std::string sign(const Json::Value &payload, const std::string &secret) {
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::FastWriter fw;
    std::string hStr = fw.write(header);
    std::string pStr = fw.write(payload);
    // FastWriter adds a trailing newline — strip it
    if (!hStr.empty() && hStr.back() == '\n') hStr.pop_back();
    if (!pStr.empty() && pStr.back() == '\n') pStr.pop_back();

    std::string sigInput = base64UrlEncode(hStr) + "." + base64UrlEncode(pStr);
    std::string sig = hmacSha256(secret, sigInput);
    return sigInput + "." + base64UrlEncode(
        reinterpret_cast<const unsigned char *>(sig.data()), sig.size());
}

struct VerifyResult {
    bool valid{false};
    std::string sub;
    std::string email;
    std::string jti;    // JWT ID — used for web cookie blacklist on logout
    std::string roles;  // roles JSON array string embedded in web JWT
    long long exp{0};
    long long iat{0};
};

// Verify and decode HS256 JWT — returns VerifyResult with valid=false on failure
inline VerifyResult verify(const std::string &token, const std::string &secret) {
    VerifyResult r;
    auto dot1 = token.find('.');
    if (dot1 == std::string::npos) return r;
    auto dot2 = token.find('.', dot1 + 1);
    if (dot2 == std::string::npos) return r;

    std::string sigInput = token.substr(0, dot2);
    std::string sigB64   = token.substr(dot2 + 1);

    // Verify signature
    std::string expectedSig = hmacSha256(secret, sigInput);
    std::string expectedB64 = base64UrlEncode(
        reinterpret_cast<const unsigned char *>(expectedSig.data()), expectedSig.size());
    if (sigB64 != expectedB64) return r;

    // Decode payload
    std::string payloadB64 = token.substr(dot1 + 1, dot2 - dot1 - 1);
    auto payloadBytes = base64UrlDecode(payloadB64);
    if (payloadBytes.empty()) return r;

    std::string payloadStr(payloadBytes.begin(), payloadBytes.end());
    Json::Value payload;
    Json::Reader reader;
    if (!reader.parse(payloadStr, payload)) return r;

    // Check expiry
    long long now = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    r.exp = payload.get("exp", Json::Value(0)).asInt64();
    if (r.exp > 0 && now > r.exp) return r; // expired

    r.sub   = payload.get("sub",   "").asString();
    r.email = payload.get("email", "").asString();
    r.jti   = payload.get("jti",   "").asString();
    r.roles = payload.get("roles", "[]").asString();
    r.iat   = payload.get("iat",   Json::Value(0)).asInt64();
    r.valid = !r.sub.empty();
    return r;
}

} // namespace jwt_helper
