#include "RateLimitFilter.h"
#include <drogon/drogon.h>
#include <drogon/HttpResponse.h>
#include <drogon/nosql/RedisClient.h>
#include <json/json.h>
#include <string>

void ratelimit::doRateLimit(const drogon::HttpRequestPtr &req,
                            drogon::FilterCallback      &&fcb,
                            drogon::FilterChainCallback &&fccb,
                            int maxRequests,
                            int windowSecs) {
    auto redis = drogon::app().getRedisClient();
    if (!redis) {
        // No Redis configured — skip rate limiting (dev mode without Redis)
        fccb();
        return;
    }

    std::string ip  = req->peerAddr().toIp();
    std::string key = "rl:" + req->path() + ":" + ip;
    int maxReq      = maxRequests;
    int winSecs     = windowSecs;

    // Wrap in shared_ptr so both lambdas can share the same callback safely
    auto fcbPtr  = std::make_shared<drogon::FilterCallback>(std::move(fcb));
    auto fccbPtr = std::make_shared<drogon::FilterChainCallback>(std::move(fccb));

    redis->execCommandAsync(
        [fcbPtr, fccbPtr, key, winSecs, maxReq]
        (const drogon::nosql::RedisResult &r) {
            long long count = 1;
            try { count = r.asInteger(); } catch (...) {}

            if (count == 1) {
                drogon::app().getRedisClient()->execCommandAsync(
                    [](const drogon::nosql::RedisResult &) {},
                    [](const std::exception &) {},
                    "EXPIRE %s %d", key.c_str(), winSecs);
            }

            if (count > maxReq) {
                Json::Value body;
                body["success"] = false;
                body["message"] = "Too many requests. Please try again later.";
                auto resp = drogon::HttpResponse::newHttpJsonResponse(body);
                resp->setStatusCode(drogon::k429TooManyRequests);
                resp->addHeader("Retry-After", std::to_string(winSecs));
                (*fcbPtr)(resp);
                return;
            }
            (*fccbPtr)();
        },
        [fccbPtr](const std::exception &) {
            // Redis error → fail open (don't block the request)
            (*fccbPtr)();
        },
        "INCR %s", key.c_str());
}
