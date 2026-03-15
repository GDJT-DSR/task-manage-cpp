#pragma once

#include <drogon/HttpController.h>
#include <string>

using namespace drogon;

namespace dto {
struct user {
    std::string username;
    std::string password;
};

struct user_change_pwd {
    std::string origin;
    std::string target;
};
} // namespace dto

namespace api {
class user : public drogon::HttpController<user> {
  public:
    METHOD_LIST_BEGIN
    METHOD_ADD(user::login, "/login",
               Post); // path is /absolute/path/{arg1}/{arg2}/list
    METHOD_ADD(user::changePassword, "/change_password", Post, );
    METHOD_ADD(user::refreshToken, "/refresh", Get);
    METHOD_ADD(user::logout, "/logout", Get);
    METHOD_LIST_END

    static Task<> login(HttpRequestPtr req,
                        std::function<void(const HttpResponsePtr &)> callback,
                        std::optional<dto::user> user);

    static Task<>
    changePassword(HttpRequestPtr req,
                   std::function<void(const HttpResponsePtr &)> callback,
                   std::optional<dto::user_change_pwd> data);

    static Task<>
    refreshToken(HttpRequestPtr req,
                 std::function<void(const HttpResponsePtr &)> callback);
    static Task<> logout(HttpRequestPtr req,
                         std::function<void(const HttpResponsePtr &)> callback);
};
} // namespace api

namespace drogon {
template <>
inline std::optional<dto::user> fromRequest(const HttpRequest &req) {
    const auto &json = req.getJsonObject();
    if (!json) {
        return std::nullopt;
    }
    const auto &username = (*json)["username"];
    const auto &password = (*json)["password"];
    if (username.isString() && password.isString()) {
        return {{username.asString(), password.asString()}};
    }
    return {};
}

template <>
inline std::optional<dto::user_change_pwd> fromRequest(const HttpRequest &req) {
    const auto &json = req.getJsonObject();
    if (!json) {
        return std::nullopt;
    }

    if (const Json::Value &origin = (*json)["origin"],
        &target = (*json)["target"];
        origin.isString() && target.isString()) {
        return dto::user_change_pwd{origin.asString(), target.asString()};
    }
    return {};
}
} // namespace drogon
