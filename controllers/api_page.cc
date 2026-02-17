#include "api_page.h"

#include <drogon/orm/CoroMapper.h>

#include "jwt-cpp/jwt.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"
#include "utils/utils.h"

using namespace api;
using namespace permission;

// Add definition of your processing function here
Task<> page::getAll(const HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback)
{
    try
    {
        const std::string& id = req->getParameter("id");
        if (id.empty())
        {
            callback(response::fail(k401Unauthorized, "获取用户失败"));
            co_return;
        }

        // 验证permission
        const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
        if (!permission::has_permission(
            permission, login | view_page))
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }

        const auto& client = app().getFastDbClient();
        if (!client)
        {
            callback(response::fail(k500InternalServerError, "服务器错误"));
            LOG_ERROR << "client is null";
        }
        const auto& res = co_await client->execSqlCoro(
            "SELECT pages.id,title,\"desc\",state,start_at,end_at FROM pages JOIN public.user_page up on pages.id = up.page_id WHERE up.user_id = $1",
            id);

        Json::Value data(Json::arrayValue);
        for (const auto& item : res)
        {
            Json::Value single;
            single["id"] = item["id"].as<int64_t>();
            single["title"] = item["title"].as<std::string>();
            single["readable"] = has_permission(item["state"].as<int64_t>(), readable);
            data.append(std::move(single));
        }

        callback(response::success(data));
        co_return;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << e.what();
    }
    catch (...)
    {
    }
    callback(response::fail(k500InternalServerError, "server error"));
    co_return;
}

Task<> page::getSingle(const HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int64_t id)
{
    try
    {
        const std::string& userid = req->getParameter("id");
        if (userid.empty())
        {
            callback(response::fail(k401Unauthorized, "获取用户失败"));
            co_return;
        }

        // 验证permission
        const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
        if (!permission::has_permission(permission, view_page))
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }

        const auto client = app().getFastDbClient();
        if (!client)
        {
            callback(response::fail(k500InternalServerError, "服务器错误"));
            LOG_ERROR << "client is null";
            co_return;
        }

        const auto pages = co_await client->execSqlCoro(
            "SELECT pages.id,title,\"desc\",state,start_at,end_at FROM pages "
            "JOIN public.user_page up on pages.id = up.page_id "
            "WHERE up.user_id = $1 AND pages.id = $2",
            userid,
            std::to_string(id)
        );
        if (pages.empty())
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }
        const auto& page = pages.front();
        const int64_t state = page["state"].as<int64_t>();
        // 验证状态
        if (permission::has_permission(state, enable | visible | readable))
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }
        const auto& questions_with_record = co_await client->execSqlCoro(
            "SELECT questions.id,title,\"desc\",type,settings,index,max_score,ans.id as ans_id,ans.content,ans.details,ans.score FROM questions "
            "LEFT JOIN (SELECT id,content,details,score,question_id FROM answers WHERE answerer_id = $2) ans "
            "ON ans.question_id = questions.id "
            "WHERE page_id = $1",
            std::to_string(id),
            userid
        );
        Json::Value data;
        data["id"] = page["id"].as<int64_t>();
        data["title"] = page["title"].as<std::string>();
        d_utils::add_to_json_if_exist(data, page, "desc");
        data["state"] = state;
        d_utils::add_to_json_if_exist(data, page, "start_at");
        d_utils::add_to_json_if_exist(data, page, "end_at");
        Json::Value arr(Json::arrayValue);

        for (const auto& question : questions_with_record)
        {
            Json::Value single;
            single["id"] = question["id"].as<int64_t>();
            single["title"] = question["title"].as<std::string>();
            d_utils::add_to_json_if_exist(single, question, "desc");
            single["type"] = question["type"].as<std::string>();
            d_utils::add_to_json_if_exist<Json::Value>(single, question, "settings");
            single["index"] = question["index"].as<int64_t>();
            single["max_score"] = question["max_score"].as<int64_t>();
            if (!question["ans_id"].isNull())
            {
                Json::Value answer;
                answer["id"] = question["ans_id"].as<int64_t>();
                d_utils::add_to_json_if_exist(answer, question, "content");
                d_utils::add_to_json_if_exist<int64_t>(answer, question, "score");
                d_utils::add_to_json_if_exist<Json::Value>(answer, question, "details");
                single["answer"] = std::move(answer);
            }
            arr.append(std::move(single));
        }
        data["questions"] = arr;
        callback(response::success(data));
        co_return;
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << e.what();
    }
    callback(response::fail(k500InternalServerError, "server error"));

    co_return;
}



