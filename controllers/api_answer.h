#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace dto
{
    struct TextSubmit
    {
        std::string target;
        std::string origin;
    };
}

namespace drogon
{
    template <>
    inline std::optional<dto::TextSubmit> fromRequest(const HttpRequest& req)
    {
        const auto& json = req.getJsonObject();
        if (!json) { return std::nullopt; }

        if (const Json::Value &target = (*json)["target"], &origin = (*json)["origin"];
            target.isString() && origin.isString()) { return dto::TextSubmit{target.asString(), origin.asString()}; }
        return std::nullopt;
    }
}


namespace api
{
    class answer : public drogon::HttpController<answer>
    {
    public:
        METHOD_LIST_BEGIN
            METHOD_ADD(answer::text, "/{id}/fill-in", Post);
            METHOD_ADD(answer::upload, "/{id}/upload", Post);
        METHOD_LIST_END

        static Task<> text(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback,
                           int question_id,
                           std::optional<dto::TextSubmit> data);
        static Task<> upload(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback, int32_t id);
        static std::string validate(const orm::Row&, const std::string&);
    };
}
