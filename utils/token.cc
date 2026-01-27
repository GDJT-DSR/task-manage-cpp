#include "token.h"
#include <drogon/utils/Utilities.h>
#include "config.h"
#include <random>
#include <shared_mutex>
#include <drogon/drogon.h>
#include <chrono>
#include "jwt-cpp/jwt.h"
#include "asio.hpp"

using namespace token;

class KeyGenerator {
    struct Key {
        const std::string &key;
        const std::string &uuid;
    };

    const size_t KEY_LENGTH = 30;
    const std::string maps{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_+=()[]{}|;:,.<>?/"};
    std::uniform_int_distribution<> dis;
    std::mt19937 gen;

    std::shared_mutex mtx;

    std::string currentKey;
    std::string currentUuid;
    asio::io_context ctx{};
    std::chrono::steady_clock::time_point expiredAt;

    Key get() {
        std::shared_lock lock(mtx);
        return {currentKey, currentUuid};
    }

    void generate() {
        std::unique_lock lock(mtx);
        if (std::chrono::steady_clock::now() < expiredAt) {
            // 别的线程更新完毕了
            return;
        }
        for (auto &c: currentKey) {
            c = maps[dis(gen)];
        }
        currentUuid = drogon::utils::getUuid();
        // 存入redis
        const auto redisClient = drogon::app().getRedisClient();
        // redisClient.return uuid;
        const int64_t duration = std::chrono::duration_cast<std::chrono::seconds>(
            config::JWT_KEY_EXPIRED + config::REFRESH_TOKEN_EXPIRED).count();

        redisClient->execCommandAsync([](const drogon::nosql::RedisResult &res) {
                                          LOG_INFO << "token存储成功";
                                      },
                                      [](const drogon::nosql::RedisException &e) {
                                          LOG_ERROR << "token存储失败";
                                      },
                                      "SET auth:token_keys:%s %s EX %lld", currentUuid.c_str(), currentKey.c_str(),
                                      duration);
        expiredAt = std::chrono::steady_clock::now() + config::JWT_KEY_EXPIRED;
    }

public:
    KeyGenerator() : dis(0, static_cast<int>(maps.size() - 1)), gen(std::random_device{}()),
                     currentKey(KEY_LENGTH, '-') {
        drogon::app().registerBeginningAdvice([this]() {
            generate();
        });
    }


    Key operator()() {
        if (std::chrono::steady_clock::now() < expiredAt) {
            // 直接返回
            return get();
        }
        generate();
        return get();
    }
} generator;

std::pair<std::string, std::string> token::generateToken(int64_t id, int64_t permission) {
    jwt::builder builder = jwt::create();
    const jwt::date &now = std::chrono::system_clock::now();
    builder.set_issued_at(now).set_expires_at(now + config::ACCESS_TOKEN_EXPIRED).set_issuer("task").set_subject("uat");
    // user access token
    const auto &[key, uuid] = generator();
    builder.set_key_id(uuid);

    picojson::value::object claim;
    claim["user_id"] = picojson::value(id);
    claim["permission"] = picojson::value(permission);
    builder.set_payload_claim("data", picojson::value(claim));
    const std::string accessToken = builder.sign(jwt::algorithm::hs256(key));

    claim.erase("permission");
    builder.set_expires_at(now + config::REFRESH_TOKEN_EXPIRED).set_payload_claim("data", picojson::value(claim));

    const std::string refreshToken = builder.sign(jwt::algorithm::hs256(key));
    // std::string accessToken = "a";
    // std::string refreshToken = "a";

    return {accessToken, refreshToken};
}


drogon::Task<std::optional<picojson::value> > getClaimJson(std::string token) {
    const auto redis = drogon::app().getRedisClient();
    if (!redis) {
        throw std::runtime_error("cannot get redis");
    }
    try {
        const auto decoded = jwt::decode(token);
        const auto kid = decoded.get_key_id();
        const drogon::nosql::RedisResult res = co_await redis->execCommandCoro(
            "GET auth:token_keys:%s", kid.c_str());
        if (res.isNil()) {
            // key已经过期
            co_return {};
        }
        const std::string key = res.asString();
        auto verifier = jwt::verify();
        verifier.with_subject("uat");
        verifier.with_issuer("task");
        verifier.allow_algorithm(jwt::algorithm::hs256(key));
        verifier.verify(decoded);
        co_return {decoded.get_payload_claim("data").to_json()};
    } catch (const std::exception &e) {
        co_return {};
    }
}

drogon::Task<std::optional<UserClaim> > token::parseAccessToken(const std::string &token) {
    const auto claim = co_await getClaimJson(token);
    if (!claim) {
        co_return {};
    }
    const auto &id = claim->get("user_id");
    const auto &permission = claim->get("permission");
    if (id.is<int64_t>() && permission.is<int64_t>()) {
        // std::unique_ptr<UserClaim> uc(new UserClaim);
        UserClaim userClaim{};

        userClaim.id = id.get<int64_t>();
        userClaim.permission = permission.get<int64_t>();
        co_return {userClaim};
    }
    co_return {};
}


drogon::Task<std::optional<int64_t> > token::parseRefreshToken(const std::string &token) {
    const auto claim = co_await getClaimJson(token);
    if (!claim) {
        co_return {};
    }
    if (const auto &id = claim->get("user_id"); id.is<int64_t>()) {
        co_return {id.get<int64_t>()};
    }
    co_return {};
}
