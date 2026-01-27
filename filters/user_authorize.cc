#include "user_authorize.h"
#include "utils/token.h"
#include "utils/response.h"
#include "jwt-cpp/jwt.h"
#include "utils/transform.h"

using namespace drogon;
using namespace user;

void authorize::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb) {
    //
    async_run([req,fcb=std::move(fcb),fccb=std::move(fccb)]()-> Task<> {
        const auto authorization = req->getHeader("Authorization");

        if (authorization.empty() || !authorization.starts_with("Bearer ")) {
            fcb(response::fail(k401Unauthorized, "未传入token"));
            co_return;
        }
        std::string token = authorization.substr(7);
        try {
            if (auto claim = co_await token::parseAccessToken(token); claim) {
                req->setParameter("id", std::to_string(claim->id));
                req->setParameter("permission", transform::int2bstring(claim->permission));
                fccb();
                co_return;
            }
            fcb(response::fail(k401Unauthorized, "token invalid"));
            co_return;
        } catch (const jwt::error::token_verification_exception &e) {
            fcb(response::fail(k401Unauthorized, e.what()));
            co_return;
        } catch (...) {
            fcb(response::fail(k500InternalServerError, "server error"));
            co_return ;
        }
    });
}
