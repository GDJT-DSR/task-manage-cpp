#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace dto
{
    struct submit
    {
        std::string content;
    };
}

namespace drogon
{
    template <>
    inline std::optional<dto::submit> fromRequest(const HttpRequest& req)
    {
        const auto& json = req.getJsonObject();
        if (!json) { return std::nullopt; }

        if (const Json::Value& content = (*json)["content"]; content.isString())
        {
            return dto::submit{content.asString()};
        }
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

        static Task<> text(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback,
                           int64_t question_id,
                           std::optional<dto::submit> submit);
        static Task<> upload(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback, int64_t id);
    };
}
