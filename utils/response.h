#pragma once

#include <functional>
#include <drogon/HttpResponse.h>

namespace response
{

    template <typename T>
    drogon::HttpResponsePtr success(T data)
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        json["data"] = data;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }
    drogon::HttpResponsePtr success()
    {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }
    drogon::HttpResponsePtr fail(drogon::HttpStatusCode code, std::string &&msg)
    {
        Json::Value json;
        json["code"] = code;
        json["msg"] = msg;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }
} // namespace response
