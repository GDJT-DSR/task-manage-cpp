#include "api_page.h"

#include <drogon/orm/CoroMapper.h>

#include "jwt-cpp/jwt.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"
#include "utils/utils.h"

using namespace api;

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
            permission, permission::user_permission::login | permission::user_permission::view_task))
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
            single["readable"] = permission::has_permission(item["state"].as<int64_t>(),
                                                            permission::task_permission::readable);
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

Task<> page::getSingle(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int64_t id)
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
        if (!permission::has_permission(permission, permission::user_permission::login))
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
        const auto& questions = co_await client->execSqlCoro(
            "SELECT questions.id,questions.title,questions.\"desc\",type,settings,index,max_score FROM questions "
            "JOIN pages ON questions.page_id = pages.id "
            "WHERE pages.id = $1",
            std::to_string(id)
        );
        const auto& page = pages.front();
        Json::Value data;
        data["id"] = page["id"].as<int64_t>();
        data["title"] = page["title"].as<std::string>();
        d_utils::add_to_json_if_exist(data, page, "desc");
        data["state"] = page["state"].as<int64_t>();
        d_utils::add_to_json_if_exist(data, page, "start_at");
        d_utils::add_to_json_if_exist(data, page, "end_at");
        Json::Value arr(Json::arrayValue);

        for (const auto& question : questions)
        {
            Json::Value single;
            single["id"] = question["id"].as<int64_t>();
            single["title"] = question["title"].as<std::string>();
            // single["desc"] = question["desc"].as<std::string>();
            d_utils::add_to_json_if_exist(single, question, "desc");
            single["type"] = question["type"].as<std::string>();
            // single["settings"] = question["settings"].as<std::string>();
            d_utils::add_to_json_if_exist(single, question, "settings");
            single["index"] = question["index"].as<int64_t>();
            single["max_score"] = question["max_score"].as<int64_t>();
            arr.append(std::move(single));
        }
        data["questions"] = arr;
        callback(response::success(data));
    }
    catch (const std::exception& e)
    {
        LOG_ERROR << e.what();
    }
    callback(response::fail(k500InternalServerError, "server error"));

    co_return;
}
