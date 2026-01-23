#pragma once

#include <functional>
#include <drogon/HttpResponse.h>

namespace response
{
    using handler = std::function<void(const HttpResponsePtr &)>;

    template <typename T>
    void success(handler &&cb, T data)
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        json["data"] = data;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        cb(resp);
    }
    void success(handler &&cb)
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        cb(resp);
    }
    void fail(handler &&cb, int code, std::string &&msg)
    {
        Json::Value json;
        json["code"] = code;
        json["msg"] = msg;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        cb(resp);
    }
} // namespace response
