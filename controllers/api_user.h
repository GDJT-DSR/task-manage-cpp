#pragma once

#include <drogon/HttpController.h>
#include <string>
#include "utils/response.h"

using namespace drogon;

namespace dto
{
  struct user
  {
    std::string username;
    std::string password;
  };

  struct user_change_pwd
  {
    std::string origin;
    std::string target;
  };

}

namespace api
{

  class user : public drogon::HttpController<user>
  {
  public:
    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    // METHOD_ADD(user::get, "/{2}/{1}", Get); // path is /api/user/{arg2}/{arg1}
    // METHOD_ADD(user::your_method_name, "/{1}/{2}/list", Get); // path is /api/user/{arg1}/{arg2}/list
    METHOD_ADD(user::login, "/login", Post); // path is /absolute/path/{arg1}/{arg2}/list
    METHOD_ADD(user::changePassword, "/change_password", Post, "user::authorize");

    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, int p1, std::string p2);
    Task<> login(const HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback, dto::user user) const;
    Task<> changePassword(const HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback, dto::user_change_pwd data) const;
  };
}

namespace drogon
{
  template <>
  dto::user fromRequest(const HttpRequest &req)
  {
    dto::user user;
    auto json = req.getJsonObject();
    if (!json)
    {
      throw response::exception::PARAM_MISMATCH;
    }
    try
    {
      user.username = (*json)["username"].asString();
      user.password = (*json)["password"].asString();
    }
    catch (...)
    {
      throw response::exception::PARAM_MISMATCH;
    }
    return user;
  }

  template <>
  dto::user_change_pwd fromRequest(const HttpRequest &req)
  {
    auto json = req.getJsonObject();
    if (!json)
    {
      throw response::exception::PARAM_MISMATCH;
    }

    try
    {
      dto::user_change_pwd dto;

      dto.origin = (*json)["origin"].asString();
      dto.target = (*json)["target"].asString();
      return dto;
    }
    catch (...)
    {
      throw response::exception::PARAM_MISMATCH;
    }
  }

} // namespace drogon
