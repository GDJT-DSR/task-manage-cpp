#pragma once

#include <drogon/HttpResponse.h>

class response {
    static drogon::HttpResponsePtr json2resp(const Json::Value &value) {
        Json::StreamWriterBuilder builder;
        builder.settings_["emitUTF8"] = true;
        builder.settings_["indentation"] = "";
        auto resp = drogon::HttpResponse::newHttpResponse();
        resp->setContentTypeCode(drogon::CT_APPLICATION_JSON);
        resp->setStatusCode(drogon::k200OK);
        resp->setBody(Json::writeString(builder, value));
        return resp;
    }

public:
    template<typename T>
    static drogon::HttpResponsePtr success(T data) {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        json["data"] = data;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        return json2resp(json);
    }

    static drogon::HttpResponsePtr success() {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        return json2resp(json);
    }

    static drogon::HttpResponsePtr fail(drogon::HttpStatusCode code, const std::string &msg) {
        Json::Value json;
        json["code"] = code;
        json["msg"] = msg;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        return json2resp(json);
    }


    class ResponsiveException : public std::exception {
        drogon::HttpStatusCode code;
        std::string msg;

    public:
        ResponsiveException(drogon::HttpStatusCode code, const std::string &msg) : code(code), msg(msg) {
        }

        ResponsiveException(drogon::HttpStatusCode code, std::string &&msg) : code(code), msg(std::move(msg)) {
        }

        ResponsiveException(const ResponsiveException &re) : code(re.code), msg(re.msg) {
        }

        ResponsiveException(ResponsiveException &&re) noexcept : code(re.code), msg(std::move(re.msg)) {
        }

        [[nodiscard]] const char *what() const noexcept override {
            return msg.c_str();
        }

        [[nodiscard]] drogon::HttpResponsePtr resp() const {
            return response::fail(code, msg);
        }
    };
}; // namespace response
