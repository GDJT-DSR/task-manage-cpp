#include "user_authorize.h"
#include "utils/token.h"
#include "utils/response.h"

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
        req->setParameter("id", std::to_string(claim->id));
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "error occured when parsing token: " << e.what() << '\n';
        co_return response::fail(k401Unauthorized, "未验证");
    }
}
