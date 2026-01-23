#pragma once

#include <functional>
#include <drogon/HttpResponse.h>

namespace response
{
    template <typename T>
    inline drogon::HttpResponsePtr success(T data)
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        json["data"] = data;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }
    inline drogon::HttpResponsePtr success()
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }
    inline drogon::HttpResponsePtr fail(drogon::HttpStatusCode code, const std::string &msg)
    {
        Json::Value json;
        json["code"] = code;
        json["msg"] = msg;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }

    namespace exception
    {
        class ResponsableException : public std::exception
        {
        private:
            drogon::HttpStatusCode code;
            std::string msg;

        public:
            ResponsableException(drogon::HttpStatusCode code, const std::string &msg) : code(code), msg(msg)
            {
            }
            ResponsableException(drogon::HttpStatusCode code, std::string &&msg) : code(code), msg(std::move(msg))
            {
            }

            const char *what() const noexcept
            {
                return msg.c_str();
            }

            drogon::HttpResponsePtr resp() const
            {
                return response::fail(code, msg);
            }
        };

        const ResponsableException PARAM_MISMATCH(drogon::k400BadRequest, "参数错误");

    }

} // namespace response
