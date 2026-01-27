#include "api_user.h"
#include "utils/response.h"
#include "models/Users.h"
#include <bcrypt/BCrypt.hpp>
#include "utils/token.h"
#include "config.h"
#include "utils/permission.h"
#include "utils/transform.h"

using namespace api;

// Add definition of your processing function here
using Users = drogon_model::task::Users;

const std::string REFRESH_TOKEN_NAME("refresh_token");

Task<> setToken(const std::function<void(const HttpResponsePtr &)> &callback, const int64_t id,
                const int64_t permission) {
    // 生成token
    const auto [accessToken, refreshToken] = token::generateToken(id, permission);

    Json::Value data;
    data["access_token"] = accessToken;

    // refresh存入cookie
    const int64_t expires = std::chrono::duration_cast<std::chrono::seconds>(config::REFRESH_TOKEN_EXPIRED).count();
    drogon::Cookie cookie("refresh_token", refreshToken);
    cookie.setMaxAge(static_cast<int>(expires));
    cookie.setSameSite(Cookie::SameSite::kStrict);
    cookie.setPath("/api/user/refresh");

    // 存入redis
    const nosql::RedisClientPtr &redis = app().getRedisClient();
    co_await redis->execCommandCoro("set auth:user_refresh_token:%lld %s EX %lld", id,
                                    refreshToken.c_str(), expires);

    const auto &resp = response::success(data);
    resp->addCookie(cookie);
    callback(resp);
}


Task<> user::login(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback,
                   std::optional<dto::user> user) {
    if (!user) {
        callback(response::fail(k400BadRequest, "param mismatch"));
        co_return;
    }
    auto client = app().getFastDbClient();
    if (!client) {
        callback(response::fail(k500InternalServerError, "server error"));
        LOG_ERROR << "connect db error!";
        co_return;
    }
    try {
        // auto result = co_await userModel.findBy({Users::Cols::_username, user->username});
        const auto &result = co_await client->execSqlCoro(
            "SELECT id,username,permission,password,updated_at FROM users WHERE username = $1 LIMIT 1", user->username);
        if (result.empty()) {
            // Response<>(400, "用户名或密码错误").respond(std::move(callback));
            callback(response::fail(k400BadRequest, "用户名或密码错误"));
            co_return;
        }
        const auto &userData = result[0];

        if (!BCrypt::validatePassword(user->password, userData["password"].as<std::string>())) {
            callback(response::fail(k400BadRequest, "用户名或密码错误"));
            co_return;
        }

        co_await setToken(callback, userData["id"].as<int64_t>(), userData["permission"].as<int64_t>());

        co_return;
    } catch (const orm::DrogonDbException &e) {
        LOG_ERROR << "connect db error: " << e.base().what();
    } catch (const nosql::RedisException &e) {
        LOG_ERROR << "connect to redis error: " << e.what();
    } catch (...) {
        LOG_ERROR << "error occurred when logining";
    }

    callback(response::fail(k500InternalServerError, "server error"));
    co_return;
}

Task<> user::changePassword(const HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback,
                            const std::optional<dto::user_change_pwd> data) {
    if (!data) {
        callback(response::fail(k400BadRequest, "param mismatch"));
        co_return;
    }
    // 读数据库
    const auto &client = drogon::app().getFastDbClient();
    if (!client) {
        callback(response::fail(k500InternalServerError, "连接数据库失败"));
    }
    try {
        std::chrono::system_clock::time_point before = std::chrono::system_clock::now();
        // 验证权限
        const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
        if (!permission::has_permission(permission, permission::user_permission::login)) {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }

        std::chrono::system_clock::time_point after = std::chrono::system_clock::now();
        LOG_INFO << "验证权限：" << std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();

        const std::string &id = req->getParameter("id");

        // auto result = co_await mapper.findBy({Users::Cols::_id, req->getParameter("id")});
        // auto result = co_await mapper.findByPrimaryKey(req->getParameter("id"));
        before = std::chrono::system_clock::now();
        const auto &result = co_await client->execSqlCoro("SELECT id,username,password FROM users WHERE id = $1",
                                                          id);
        if (result.empty()) {
            callback(response::fail(k400BadRequest, "找不到用户"));
            co_return;
        }
        after = std::chrono::system_clock::now();
        LOG_INFO << "查库：" << std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();


        const orm::Row &userData = result[0];
        before = std::chrono::system_clock::now();
        if (!BCrypt::validatePassword(data->origin, userData["password"].as<std::string>())) {
            callback(response::fail(k400BadRequest, "用户名或密码错误"));
            co_return;
        }
        after = std::chrono::system_clock::now();


        before = std::chrono::system_clock::now();
        std::string encryptedPassword = BCrypt::generateHash(data->target, 10);
        after = std::chrono::system_clock::now();


        before = std::chrono::system_clock::now();
        const auto Result = co_await client->execSqlCoro("UPDATE users SET password=$1 WHERE id=$2", encryptedPassword,
                                                         id);
        after = std::chrono::system_clock::now();
        LOG_INFO << "存储密码：" << std::chrono::duration_cast<std::chrono::milliseconds>(after - before).count();

        if (result.empty()) {
            callback(response::fail(k500InternalServerError, "更新失败"));
        }
        callback(response::success());
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
    } catch (...) {
        LOG_ERROR << "error occurred when changing password";
    }
    callback(response::fail(k500InternalServerError, "服务器错误"));

    co_return;
}

Task<> user::refreshToken(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) {
    try {
        // 获取 refresh token
        const std::string &token_string = req->getCookie("refresh_token");

        const auto &id = co_await token::parseRefreshToken(token_string);
        if (!id) {
            // token 未验证
            callback(response::fail(k401Unauthorized, "invalid token"));
            co_return;
        }
        // 验证成功
        const auto &client = drogon::app().getFastDbClient();
        if (!client) {
            LOG_ERROR << "fail to connect to database";
            callback(response::fail(k500InternalServerError, "server error"));
            co_return;
        }
        orm::CoroMapper<Users> mapper(client);
        const auto &users = co_await mapper.findBy({Users::Cols::_id, id.value()});
        if (users.empty()) {
            callback(response::fail(k401Unauthorized, "user not found"));
            co_return;
        }
        const auto user = users[0];
        co_await setToken(callback, *user.getId(), *user.getPermission());

        co_return;
    } catch (const std::exception &e) {
        LOG_ERROR << e.what();
    } catch (const drogon::orm::DrogonDbException &e) {
        LOG_ERROR << e.base().what();
    } catch (...) {
        LOG_ERROR << "error occurred when refreshing token";
    }
    response::fail(k500InternalServerError, "server error");
    co_return;
}


