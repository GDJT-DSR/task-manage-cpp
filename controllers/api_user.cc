#include "api_user.h"
#include "utils/response.h"
#include "models/Users.h"
#include <bcrypt/BCrypt.hpp>
#include "utils/token.h"
#include "config.h"

using namespace api;

// Add definition of your processing function here

Task<> api::user::login(const HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback, dto::user user) const
{
    using Users = drogon_model::task::Users;
    auto client = drogon::app().getFastDbClient();
    orm::CoroMapper<Users> userModel(client);
    try
    {
        auto result = co_await userModel.findBy({Users::Cols::_username, user.username});
        if (result.size() != 1)
        {
            // Response<>(400, "用户名或密码错误").respond(std::move(callback));
            callback(response::fail(k400BadRequest, "用户名或密码错误"));
            co_return;
        }
        const Users &userData = result[0];
        std::int64_t id = *userData.getId();
        if (!BCrypt::validatePassword(user.password, *userData.getPassword()))
        {
            callback(response::fail(k400BadRequest, "用户名或密码错误"));
            co_return;
        }
        // 生成token
        const auto tokenPair = token::generateToken(*userData.getId(), *userData.getPermission());
        // const auto tokenPair = std::pair("a", "a");

        Json::Value data;
        data["access_token"] = tokenPair.first;

        // refresh存入cookie
        const int64_t expires = std::chrono::duration_cast<std::chrono::seconds>(config::REFRESH_TOKEN_EXPIRED).count();
        drogon::Cookie cookie("refresh_token", tokenPair.second);
        cookie.setMaxAge(expires);
        cookie.setSameSite(Cookie::SameSite::kStrict);
        cookie.setPath("/api/v1/user/refresh");

        // 存入redis
        drogon::nosql::RedisClientPtr redis = app().getRedisClient();
        co_await redis->execCommandCoro("set auth:user_refresh_token:%lld %s EX %lld", *userData.getId(), tokenPair.second.c_str(), expires);

        // callback(response::success([&](const HttpResponsePtr &resp)
        //                            {
        //                      resp->addCookie(cookie);
        //                      callback(resp); }, data));
        auto resp = response::success(data);
        resp->addCookie(cookie);
        callback(resp);

        co_return;
    }
    catch (const orm::DrogonDbException &e)
    {
        LOG_ERROR << "查询数据库失败";
    }
    catch (const nosql::RedisException &e)
    {
        LOG_ERROR << "redis查询失败";
    }
    catch (...)
    {
    }

    callback(response::fail(k500InternalServerError, "服务器错误"));
    co_return;
}
