#include "token.h"
#include <drogon/utils/Utilities.h>
// #include "jwt-cpp/jwt.h"
#include "config.h"
#include <random>
#include <shared_mutex>
#include <drogon/drogon.h>
#include <drogon/utils/Utilities.h>
#include <chrono>
#include "utils/timer.h"

using namespace token;

class KeyGenerator
{
    const size_t KEY_LENGTH = 30;
    const std::string maps{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_+=()[]{}|;:,.<>?/"};
    std::uniform_int_distribution<> dis;
    std::mt19937 gen;

    std::shared_mutex mtx;

    std::string currentKey;
    std::string currentUuid;

    timer::interval work;

public:
    KeyGenerator() : dis(0, maps.size() - 1), gen(std::random_device{}()), currentKey(KEY_LENGTH, '?'), work([this]()
                                                                                                             { this->generate(); }, config::JWT_KEY_EXPIRED)
    {
        drogon::app().registerBeginningAdvice([this]()
                                              { work.start(); });
    }

    void generate()
    {
        std::unique_lock lock(mtx);
        mtx.lock();
        for (auto &c : currentKey)
        {
            c = maps[dis(gen)];
        }
        currentUuid = drogon::utils::getUuid();
        // 存入redis
        auto redisClient = drogon::app().getRedisClient();
        // redisClient.return uuid;
        const int64_t duration = std::chrono::duration_cast<std::chrono::seconds>(config::JWT_KEY_EXPIRED + config::REFRESH_TOKEN_EXPIRED).count();

        redisClient->execCommandAsync([this](const drogon::nosql::RedisResult &res)
                                      { LOG_ERROR << "token存储成功"; },
                                      [this](const drogon::nosql::RedisException &e)
                                      { LOG_ERROR << "token存储失败"; },
                                      "SET auth:token_keys:%s %s EX %lld", currentUuid.c_str(), currentKey.c_str(), duration);
    }

    struct Key
    {
        std::string key;
        std::string uuid;
    };
    Key operator()()
    {
        std::shared_lock lock(mtx);
        return {.key = currentKey, .uuid = currentUuid};
    }
} generator;

std::pair<std::string, std::string> token::generateToken(int64_t id, int64_t permission)
{
    // jwt::builder builder = jwt::create();
    // jwt::date now = std::chrono::system_clock::now();
    // builder.set_issued_at(now).set_expires_at(now + config::ACCESS_TOKEN_EXPIRED).set_issuer("task").set_subject("uat"); // user access token
    // KeyGenerator::Key key = generator();
    // builder.set_key_id(key.uuid);

    // picojson::value::object claim;
    // claim["user_id"] = picojson::value(id);
    // claim["permission"] = picojson::value(permission);
    // builder.set_payload_claim("data", picojson::value(claim));
    // const std::string accessToken = builder.sign(jwt::algorithm::hs256(key.key));

    // claim.erase("permission");
    // builder.set_expires_at(now + config::REFRESH_TOKEN_EXPIRED).set_payload_claim("data", picojson::value(claim));

    // const std::string refreshToken = builder.sign(jwt::algorithm::hs256(key.key));
    std::string accessToken = "a";
    std::string refreshToken = "a";

    return std::pair<std::string, std::string>{accessToken, refreshToken};
}

drogon::Task<UserClaim> token::parseAccessToken(std::string token)
{
    UserClaim uc;
    // auto decoded = jwt::decode(token);
    co_return uc;
}
