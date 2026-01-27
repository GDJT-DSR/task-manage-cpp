#pragma once

#include <drogon/HttpResponse.h>

namespace response {
    template<typename T>
    inline drogon::HttpResponsePtr success(T data) {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        json["data"] = data;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }

    inline drogon::HttpResponsePtr success() {
        Json::Value json;
        json["code"] = 200;
        json["msg"] = "ok";
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }

    inline drogon::HttpResponsePtr fail(drogon::HttpStatusCode code, const std::string &msg) {
        Json::Value json;
        json["code"] = code;
        json["msg"] = msg;
        auto resp = drogon::HttpResponse::newHttpJsonResponse(json);
        // cb(resp);
        return std::move(resp);
    }

    namespace exception {
        class ResponsiveException : public std::exception {
        private:
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

        const ResponsiveException PARAM_MISMATCH(drogon::k400BadRequest, "参数错误");
    }
} // namespace response
