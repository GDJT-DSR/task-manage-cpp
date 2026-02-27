#include "token.h"
#include <drogon/utils/Utilities.h>
#include "config.h"
#include <random>
#include <shared_mutex>
#include <drogon/drogon.h>
#include <chrono>
#include <utility>
#include "jwt-cpp/jwt.h"
#include "asio.hpp"
#include "Timer.h"

using namespace token;

class KeyGenerator
{
    struct Key
    {
        const std::string& key;
        const std::string& uuid;
    };

    const size_t KEY_LENGTH = 30;
    const std::string maps{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_+=()[]{}|;:,.<>?/"};
    std::uniform_int_distribution<> dis;
    std::mt19937 gen;

    std::shared_mutex mtx;

    std::string currentKey;
    std::string currentUuid;
    Timer timer;

    Key get()
    {
        std::shared_lock lock(mtx);
        return {currentKey, currentUuid};
    }

    void generate()
    {
        std::unique_lock lock(mtx);
        for (auto& c : currentKey) { c = maps[dis(gen)]; }
        currentUuid = drogon::utils::getUuid();
        // redisClient.return uuid;
        constexpr int64_t duration = std::chrono::duration_cast<std::chrono::seconds>(
            config::JWT_KEY_EXPIRED + config::REFRESH_TOKEN_EXPIRED).count();
        // 存入redis
        drogon::app().getLoop()->queueInLoop([this]() {
            const auto client = drogon::app().getFastRedisClient();
            if (!client)
            {
                LOG_ERROR << "client is null";
                throw std::runtime_error("client is null");
            }
            client->execCommandAsync([](const drogon::nosql::RedisResult&) { LOG_INFO << "token存储成功"; },
                                     [](const drogon::nosql::RedisException&) { LOG_ERROR << "token存储失败"; },
                                     "SET auth:token_keys:%s %s EX %lld",
                                     this->currentUuid.c_str(),
                                     this->currentKey.c_str(),
                                     duration);
        });
    }

public:
    KeyGenerator() : dis(0, static_cast<int>(maps.size() - 1)), gen(std::random_device{}()),
                     currentKey(KEY_LENGTH, '-'), timer(config::JWT_KEY_EXPIRED)
    {
        drogon::app().registerBeginningAdvice([this]() {
            generate();
            this->timer.start([this] {
                generate();
                std::cout << "abc" << std::endl;
            });
        });
    }


    Key operator()() { return get(); }
} generator;

std::pair<std::string, std::string>
token::generateToken(int64_t id, int64_t permission, const std::string& updated_at)
{
    jwt::builder builder = jwt::create();
    const jwt::date& now = std::chrono::system_clock::now();
    builder.set_issued_at(now).set_expires_at(now + config::ACCESS_TOKEN_EXPIRED).set_type("access");
    // user access token
    const auto& [key, uuid] = generator();
    builder.set_key_id(uuid);

    picojson::value::object claim;
    claim["user_id"] = picojson::value(id);
    claim["permission"] = picojson::value(permission);
    builder.set_payload_claim("data", picojson::value(claim));
    builder.set_issuer("task");
    builder.set_subject("uat");
    // verifier.with_subject("uat");
    // verifier.with_issuer("task");
    const std::string accessToken = builder.sign(jwt::algorithm::hs256(key));

    claim.erase("permission");
    claim["updated_at"] = picojson::value(updated_at);

    builder.set_subject("urt").set_expires_at(now + config::REFRESH_TOKEN_EXPIRED).set_payload_claim(
        "data", picojson::value(claim));

    const std::string refreshToken = builder.sign(jwt::algorithm::hs256(key));
    // std::string accessToken = "a";
    // std::string refreshToken = "a";

    return {accessToken, refreshToken};
}


drogon::Task<std::optional<picojson::value>> getClaimJson(const std::string& token, const std::string& subject)
{
    size_t idx = drogon::app().getCurrentThreadIndex();
    const auto redis = drogon::app().getFastRedisClient();
    if (!redis) { throw std::runtime_error("cannot get redis"); }
    try
    {
        const auto decoded = jwt::decode(token);
        const auto kid = decoded.get_key_id();
        const drogon::nosql::RedisResult res = co_await redis->execCommandCoro(
            "GET auth:token_keys:%s", kid.c_str());
        if (res.isNil())
        {
            // key已经过期
            co_return std::nullopt;
        }
        const std::string key = res.asString();
        auto verifier = jwt::verify();
        verifier.with_subject(subject);
        verifier.with_issuer("task");
        verifier.allow_algorithm(jwt::algorithm::hs256(key));
        verifier.verify(decoded);
        co_await drogon::switchThreadCoro(drogon::app().getIOLoop(idx));
        co_return decoded.get_payload_claim("data").to_json();
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (...) {}
    co_return std::nullopt;
}

drogon::Task<std::optional<UserClaim>> token::parseAccessToken(const std::string& token)
{
    const auto claim = co_await getClaimJson(token, "uat");
    if (!claim) { co_return std::nullopt; }
    const auto& id = claim->get("user_id");
    const auto& permission = claim->get("permission");
    if (id.is<int64_t>() && permission.is<int64_t>())
    {
        // std::unique_ptr<UserClaim> uc(new UserClaim);
        UserClaim userClaim{};

        userClaim.id = static_cast<int>(id.get<int64_t>());
        userClaim.permission = permission.get<int64_t>();
        co_return userClaim;
    }
    co_return std::nullopt;
}


drogon::Task<std::optional<std::pair<int, std::string>>> token::parseRefreshToken(const std::string& token)
{
    const auto claim = co_await getClaimJson(token, "urt");
    if (!claim) { co_return std::nullopt; }
    const auto& id = claim->get("user_id");
    const auto& updated_at = claim->get("updated_at");
    if (id.is<int64_t>() && updated_at.is<std::string>())
    {
        co_return {{id.get<int64_t>(), updated_at.get<std::string>()}};
    }
    co_return std::nullopt;
}
