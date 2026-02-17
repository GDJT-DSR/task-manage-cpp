#include "api_user.h"
#include "config.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/token.h"
#include "utils/transform.h"
#include <bcrypt/BCrypt.hpp>

using namespace api;

// Add definition of your processing function here

const std::string REFRESH_TOKEN_NAME("refresh_token");

Task<> setToken(const std::function<void(const HttpResponsePtr&)>& callback,
                const int64_t id, const int64_t permission,
                const std::string& updated_at)
{
  // 生成token
  const auto [accessToken, refreshToken] =
    token::generateToken(id, permission, updated_at);

  Json::Value data;
  data["access_token"] = accessToken;

  // refresh存入cookie
  const int64_t expires = std::chrono::duration_cast<std::chrono::seconds>(
      config::REFRESH_TOKEN_EXPIRED)
    .count();
  drogon::Cookie cookie("refresh_token", refreshToken);
  cookie.setMaxAge(static_cast<int>(expires));
  cookie.setSameSite(Cookie::SameSite::kStrict);
  cookie.setPath("/api/user/refresh");

  // 存入redis
  const nosql::RedisClientPtr& redis = app().getFastRedisClient();
  co_await redis->execCommandCoro("set auth:user_refresh_token:%lld %s EX %lld",
                                  id, refreshToken.c_str(), expires);

  const auto& resp = response::success(data);
  resp->addCookie(cookie);
  callback(resp);
}

Task<> user::login(HttpRequestPtr req,
                   std::function<void(const HttpResponsePtr&)> callback,
                   std::optional<dto::user> user)
{
  if (!user)
  {
    callback(response::fail(k400BadRequest, "param mismatch"));
    co_return;
  }
  const auto client = app().getFastDbClient();
  if (!client)
  {
    callback(response::fail(k500InternalServerError, "server error"));
    LOG_ERROR << "connect db error!";
    co_return;
  }
  try
  {
    // auto result = co_await userModel.findBy({Users::Cols::_username,
    // user->username});
    const auto& result = co_await client->execSqlCoro(
      "SELECT id,username,permission,password,updated_at FROM users WHERE "
      "username = $1 LIMIT 1",
      user->username);
    if (result.empty())
    {
      // Response<>(400, "用户名或密码错误").respond(std::move(callback));
      callback(response::fail(k400BadRequest, "用户名或密码错误"));
      co_return;
    }
    const auto& userData = result[0];

    if (!BCrypt::validatePassword(user->password,
                                  userData["password"].as<std::string>()))
    {
      callback(response::fail(k400BadRequest, "用户名或密码错误"));
      co_return;
    }

    co_await setToken(callback, userData["id"].as<int64_t>(),
                      userData["permission"].as<int64_t>(),
                      userData["updated_at"].as<std::string>());

    co_return;
  }
  catch (const orm::DrogonDbException& e)
  {
    LOG_ERROR << "connect db error: " << e.base().what();
  }
  catch (const nosql::RedisException& e)
  {
    LOG_ERROR << "connect to redis error: " << e.what();
  }
  catch (...)
  {
    LOG_ERROR << "error occurred when logining";
  }

  callback(response::fail(k500InternalServerError, "server error"));
  co_return;
}

Task<>
user::changePassword(const HttpRequestPtr req,
                     std::function<void(const HttpResponsePtr&)> callback,
                     const std::optional<dto::user_change_pwd> data)
{
  if (!data)
  {
    callback(response::fail(k400BadRequest, "param mismatch"));
    co_return;
  }
  // 读数据库
  const auto& client = drogon::app().getFastDbClient();
  if (!client)
  {
    LOG_ERROR << "connect db error!";
    callback(response::fail(k500InternalServerError, "server error"));
  }
  try
  {
    // 验证权限
    const auto permission =
      transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!permission::has_permission(permission,
                                    permission::user_permission::login))
    {
      callback(response::fail(k403Forbidden, "权限不足"));
      co_return;
    }

    const std::string& id = req->getParameter("id");

    // auto result = co_await mapper.findBy({Users::Cols::_id,
    // req->getParameter("id")}); auto result = co_await
    // mapper.findByPrimaryKey(req->getParameter("id"));
    const auto& result = co_await client->execSqlCoro(
      "SELECT id,username,password FROM users WHERE id = $1", id);
    if (result.empty())
    {
      callback(response::fail(k400BadRequest, "找不到用户"));
      co_return;
    }

    const orm::Row& userData = result[0];
    if (!BCrypt::validatePassword(data->origin,
                                  userData["password"].as<std::string>()))
    {
      callback(response::fail(k400BadRequest, "用户名或密码错误"));
      co_return;
    }

    std::string encryptedPassword = BCrypt::generateHash(data->target, 10);

    const auto Result = co_await client->execSqlCoro(
      "UPDATE users SET password=$1 WHERE id=$2", encryptedPassword, id);

    if (result.empty())
    {
      callback(response::fail(k500InternalServerError, "更新失败"));
    }
    callback(response::success());
  }
  catch (const std::exception& e)
  {
    LOG_ERROR << e.what();
  }
  catch (...)
  {
    LOG_ERROR << "error occurred when changing password";
  }
  callback(response::fail(k500InternalServerError, "服务器错误"));

  co_return;
}

Task<>
user::refreshToken(const HttpRequestPtr req,
                   const std::function<void(const HttpResponsePtr&)> callback)
{
  const std::string& token_string = req->getCookie("refresh_token");
  const auto client = app().getFastDbClient();
  if (!client)
  {
    callback(response::fail(k500InternalServerError, "server error"));
    LOG_ERROR << "connect db error!";
    co_return;
  }

  const auto& token = co_await token::parseRefreshToken(token_string);
  if (!token)
  {
    // token 未验证
    callback(response::fail(k401Unauthorized, "invalid token"));
    co_return;
  }

  // const auto& [id, updated_at_from_token] = token.value();
  const auto& [id, updated_at_from_token] = token.value();
  // 验证成功

  try
  {
    // 获取 refresh token

    // const auto &users = co_await mapper.findBy({Users::Cols::_id,
    // id.value()});
    const auto& result =
      co_await client->execSqlCoro("SELECT * FROM users WHERE id = $1", std::to_string(id));
    if (result.size() != 1)
    {
      callback(response::fail(k403Forbidden, "user not found"));
      co_return;
    }
    const auto& user = result[0];
    const auto& updated_at_from_db = user["updated_at"].as<std::string>();
    if (updated_at_from_db != updated_at_from_token)
    {
      callback(response::fail(k403Forbidden, "user too old"));
      co_return;
    }

    co_await setToken(callback, user["id"].as<int64_t>(),
                      user["permission"].as<int64_t>(), updated_at_from_db);

    co_return;
  }
  catch (const std::exception& e)
  {
    LOG_ERROR << e.what();
  }
  catch (const drogon::orm::DrogonDbException& e)
  {
    LOG_ERROR << e.base().what();
  }
  catch (...)
  {
    LOG_ERROR << "error occurred when refreshing token";
  }
  callback(response::fail(k500InternalServerError, "server error"));
  co_return;
}
