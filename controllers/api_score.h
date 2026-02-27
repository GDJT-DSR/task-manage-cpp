#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace dto
{
    struct Score
    {
        int target{};
        int origin{};
    };
}

namespace api
{
    class score : public drogon::HttpController<score>
    {
    public:
        METHOD_LIST_BEGIN
            METHOD_ADD(score::getAll, "", Get);
            METHOD_ADD(score::getSingle, "/{id}", Get);
            METHOD_ADD(score::submitScore, "/{id}", Post);
        METHOD_LIST_END

        static Task<> getAll(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback);
        static Task<> getSingle(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int id);
        static Task<> submitScore(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int id,
                                  std::optional<dto::Score> score);
        static bool validateScore(int val, const orm::Row& row);
    };
}

namespace drogon
{
    template <>
    inline std::optional<dto::Score> fromRequest(const HttpRequest& req)
    {
        using dto::Score;
        const auto& json = req.getJsonObject();
        if (!json) { return std::nullopt; }
        const auto& target = (*json)["target"];
        const auto& origin = (*json)["origin"];
        if (!target.isInt() || !origin.isInt()) { return std::nullopt; }
        return Score{target.asInt(), origin.asInt()};
    }
}
