#pragma once

#include <drogon/HttpResponse.h>

using namespace drogon;

class response
{
public:
    template <typename T>
    static HttpResponsePtr success(T data)
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        json["data"] = data;
        return HttpResponse::newHttpJsonResponse(json);
    }

    static HttpResponsePtr success()
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        return HttpResponse::newHttpJsonResponse(json);
    }

    static HttpResponsePtr fail(HttpStatusCode code, const std::string& msg)
    {
        Json::Value json;
        json["code"] = code;
        json["msg"] = msg;
        return HttpResponse::newHttpJsonResponse(json);
    }


    class ResponsiveException : public std::exception
    {
        HttpStatusCode code;
        std::string msg;

    public:
        ResponsiveException(HttpStatusCode code, const std::string& msg) : code(code), msg(msg) {}

        ResponsiveException(HttpStatusCode code, std::string&& msg) : code(code), msg(std::move(msg)) {}

        ResponsiveException(const ResponsiveException& re) : code(re.code), msg(re.msg) {}

        ResponsiveException(ResponsiveException&& re) noexcept : code(re.code), msg(std::move(re.msg)) {}

        [[nodiscard]] const char* what() const noexcept override { return msg.c_str(); }

        [[nodiscard]] HttpResponsePtr resp() const { return response::fail(code, msg); }
    };
}; // namespace response
