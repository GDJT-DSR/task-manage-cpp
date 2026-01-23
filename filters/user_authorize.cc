#include "user_authorize.h"
#include "utils/token.h"
#include "utils/response.h"
#include "jwt-cpp/jwt.h"

using namespace drogon;
using namespace user;

Task<HttpResponsePtr> user::authorize::doFilter(const HttpRequestPtr &req)
{

    const auto authorization = req->getHeader("Authorization");

    if (authorization.empty() || !authorization.starts_with("Bearer "))
    {
        co_return response::fail(k401Unauthorized, "未传入token");
    }
    std::string token = authorization.substr(7);
    try
    {
        auto claim = co_await token::parseAccessToken(token);
        if (claim)
        {
            req->setParameter("id", std::to_string(claim->id));
            co_return nullptr;
        }
        else
        {
            co_return response::fail(k401Unauthorized, "unsupported token");
        }
    }
    catch (const jwt::error::token_verification_exception &e)
    {
        co_return response::fail(k401Unauthorized, e.what());
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << e.what();
        co_return response::fail(k500InternalServerError, "server error");
    }
    catch (...)
    {
        co_return response::fail(k500InternalServerError, "server error");
    }
}
