#include "api_page.h"

#include <drogon/orm/CoroMapper.h>

#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"

using namespace api;

// Add definition of your processing function here
Task<> page::getAll(const HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) {
    const std::string &id = req->getParameter("id");
    if (id.empty()) {
        callback(response::fail(k401Unauthorized, "获取用户失败"));
        co_return;
    }

    // 验证permission
    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!permission::has_permission(
        permission, permission::user_permission::login | permission::user_permission::view_task)) {
        callback(response::fail(k403Forbidden, "权限不足"));
        co_return;
    }

    const auto &client = app().getFastDbClient();
    if (!client) {
        callback(response::fail(k500InternalServerError, "服务器错误"));
        LOG_ERROR << "client is null";
    }
    const auto &res = co_await client->execSqlCoro(
        "SELECT * FROM tasks INNER JOIN user_tasks on tasks.id = user_tasks.task_id WHERE user_tasks.user_id=$1 AND tasks.deleted_at IS NULL",
        id);

    Json::Value data(Json::arrayValue);
    for (const auto &item: res) {
        Json::Value single;
        single["id"] = item["id"].as<int64_t>();
        single["title"] = item["title"].as<std::string>();
        single["desc"] = item["desc"].as<std::string>();
        // single["start_at"] = item["start_at"].as<std::string>();
        // single["end_at"] = item["end_at"].as<std::string>();
        single["readable"] = permission::has_permission(item["state"].as<int64_t>(),
                                                        permission::task_permission::readable);
        data.append(std::move(single));
    }

    callback(response::success(data));
    co_return;
}

Task<> page::getSingle(HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback, int64_t id) {
    const std::string &userid = req->getParameter("id");
    if (userid.empty()) {
        callback(response::fail(k401Unauthorized, "获取用户失败"));
        co_return;
    }

    // 验证permission
    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!permission::has_permission(permission, permission::user_permission::login)) {
        callback(response::fail(k403Forbidden, "权限不足"));
        co_return;
    }

    const auto &client = app().getFastDbClient();
    if (!client) {
        callback(response::fail(k500InternalServerError, "服务器错误"));
        LOG_ERROR << "client is null";
        co_return;
    }
    const auto &conn = co_await client->execSqlCoro("SELECT * FROM user_page WHERE user_id=$1 AND page_id=$2", userid,
                                                    id);
    const auto &pages = co_await client->execSqlCoro("SELECT * FROM pages WHERE id=$1", id);
    if (conn.size() != 1 || pages.size() != 1) {
        callback(response::fail(k403Forbidden, "权限不足"));
        co_return;
    }
    const auto &page = pages[0];
    Json::Value data;
    data["id"] = page["id"].as<int64_t>();
    data["title"] = page["title"].as<std::string>();
    data["desc"] = page["desc"].as<std::string>();
    data["state"] = page["state"].as<int64_t>();
    data["start_at"] = page["start_at"].as<std::string>();
    data["end_at"] = page["end_at"].as<std::string>();
    Json::Value arr(Json::arrayValue);
    const auto &questions = co_await client->execSqlCoro("SELECT * FROM questions WHERE page_id=$1", id);
    const auto &answers = co_await client->execSqlCoro("SELECT * FROM answers WHERE page_id=$1", id);


    data["sub_tasks"] = arr;
}
