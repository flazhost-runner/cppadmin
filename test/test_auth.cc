#include <drogon/drogon_test.h>
#include "../vendor/bcrypt/bcrypt.h"
#include "../include/helpers/JwtHelper.h"

// ── Unit: bcrypt ──────────────────────────────────────────────────────────────
DROGON_TEST(BcryptRoundtrip) {
    std::string pw   = "12345678";
    std::string hash = bcrypt::hash(pw, 4); // rounds=4 for speed in tests
    CHECK(!hash.empty());
    CHECK(hash.substr(0, 4) == "$2b$");
    CHECK(bcrypt::verify(pw, hash));
    CHECK(!bcrypt::verify("wrong", hash));
    CHECK(!bcrypt::verify("", hash));
}

DROGON_TEST(BcryptEmptyPassword) {
    std::string hash = bcrypt::hash("", 4);
    CHECK(!hash.empty());
    CHECK(bcrypt::verify("", hash));
    CHECK(!bcrypt::verify("x", hash));
}

// ── Unit: JWT helper ──────────────────────────────────────────────────────────
DROGON_TEST(JwtSignAndVerify) {
    std::string secret = "test-secret-key-32-chars-ok!1234";
    Json::Value payload;
    payload["sub"]   = "user-123";
    payload["email"] = "test@test.com";
    payload["exp"]   = (Json::Int64)9999999999LL; // far future

    std::string token = jwt_helper::sign(payload, secret);
    CHECK(!token.empty());
    // Token has 3 parts: header.payload.signature
    int dots = 0;
    for (char c : token) if (c == '.') ++dots;
    CHECK(dots == 2);

    // Verify with correct secret
    auto r = jwt_helper::verify(token, secret);
    CHECK(r.valid);
    CHECK(r.sub   == "user-123");
    CHECK(r.email == "test@test.com");

    // Tampered token should fail
    std::string tampered = token.substr(0, token.size() - 3) + "AAA";
    CHECK(!jwt_helper::verify(tampered, secret).valid);

    // Wrong secret should fail
    CHECK(!jwt_helper::verify(token, "wrong-secret").valid);
}

DROGON_TEST(JwtExpired) {
    std::string secret = "test-secret-key-32-chars-ok!1234";
    Json::Value payload;
    payload["sub"] = "user-expired";
    payload["exp"] = (Json::Int64)1L; // Unix epoch + 1 second — long expired

    std::string token = jwt_helper::sign(payload, secret);
    CHECK(!token.empty());
    // Expired tokens should not verify
    CHECK(!jwt_helper::verify(token, secret).valid);
}
