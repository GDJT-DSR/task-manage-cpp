#include "api_score.h"

#include <ranges>
#include <drogon/orm/Criteria.h>
#include <drogon/orm/DbClient.h>

#include "utils/add_to_json.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"

using namespace api;

// Add definition of your processing function here
Task<> score::getAll(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback)
{
    const auto& uid = req->getParameter("id");
    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!permission::has_permission(permission, permission::user_permission::score))
    {
        callback(response::fail(k403Forbidden, "权限不足"));
    }
    const auto client = app().getFastDbClient();
    if (!client)
    {
        callback(response::fail(k500InternalServerError, "服务器错误"));
        LOG_ERROR << "client is null";
        co_return;
    }
    try
    {
        auto questions = co_await client->execSqlCoro(
            "SELECT questions.id,title,type,page_id FROM questions "
            "JOIN scorer_question ON questions.id = scorer_question.question_id "
            "WHERE scorer_question.user_id = $1",
            uid
        );
        if (questions.empty()) { callback(response::success(Json::Value(Json::arrayValue))); }
        std::unordered_map<int64_t, Json::Value> map;
        for (const auto& question : questions)
        {
            const int64_t pid = question["page_id"].as<int64_t>();
            if (!map.contains(pid)) { map.insert({pid, Json::Value(Json::arrayValue)}); }
            Json::Value val;
            val["id"] = question["id"].as<int64_t>();
            val["title"] = question["title"].as<std::string>();
            val["type"] = question["type"].as<std::string>();
            map[pid].append(std::move(val));
        }

        std::string clause("SELECT id,title,state FROM pages WHERE id IN (");
        clause.reserve(map.size() * 3 + 50);
        std::vector<std::string> pids;
        pids.reserve(map.size());
        int index = 0;
        for (const int64_t key : map | std::views::keys)
        {
            clause.push_back('$');
            clause.append(std::to_string(++index));
            clause.push_back(',');
            pids.emplace_back(std::to_string(key));
        }
        clause.back() = ')';

        // 查询page
        const auto pages = co_await client->execSqlCoro<std::string>(clause, pids);
        Json::Value resp(Json::arrayValue);
        for (const auto& page : pages)
        {
            const int64_t state = page["state"].as<int64_t>();
            if (!permission::has_permission(state,
                                            permission::enable | permission::reviewer_visible)) { continue; }
            Json::Value single;
            int64_t id = page["id"].as<int64_t>();
            single["id"] = id;
            single["title"] = page["title"].as<std::string>();
            single["reviewable"] = has_permission(state, permission::reviewable);
            single["questions"] = map[id];
            resp.append(std::move(single));
        }
        callback(response::success(resp));
        co_return;
    }
    catch
    (const drogon::orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); }
    catch (...) { LOG_ERROR << "unknown exception"; }
    callback(response::fail(k500InternalServerError, "服务器错误"));
    co_return;
}

Task<> score::getSingle(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int64_t id)
{
    const auto qid = std::to_string(id);
    const auto& uid = req->getParameter("id");
    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!permission::has_permission(permission, permission::user_permission::score))
    {
        callback(response::fail(k403Forbidden, "权限不足"));
    }
    const auto client = app().getFastDbClient();
    if (!client)
    {
        callback(response::fail(k500InternalServerError, "服务器错误"));
        LOG_ERROR << "client is null";
        co_return;
    }
    try
    {
        // 验证权限
        const auto questions = co_await client->execSqlCoro(
            "SELECT type,max_score,score_step,state FROM questions "
            "JOIN pages ON pages.id = page_id "
            "WHERE questions.id = $1",
            std::to_string(id)
        );
        if (questions.empty()) { callback(response::fail(k403Forbidden, "权限不足")); }
        const auto& question = questions.front();
        const auto type = question["type"].as<std::string>();
        const int64_t state = question["state"].as<int64_t>();
        if (!has_permission(state, permission::score)) { callback(response::fail(k403Forbidden, "权限不足")); }
        Json::Value resp;
        resp["type"] = type;
        resp["max_score"] = question["max_score"].as<int64_t>();
        resp["score_step"] = question["score_step"].as<double>();
        Json::Value arr(Json::arrayValue);
        const auto answers = co_await client->execSqlCoro(
            "SELECT content,details,score from answers WHERE scorer_id = $1 AND question_id = $2",
            uid,
            qid
        );
        for (const auto& answer : answers)
        {
            Json::Value val;
            val["content"] = answer["content"].as<std::string>();
            val["details"] = answer["details"].as<Json::Value>();
            val["score"] = answer["score"].as<int64_t>();
            arr.append(std::move(val));
        }
        resp["answers"] = std::move(arr);
        callback(response::success(resp));
        co_return;
    }
    catch (const drogon::orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); }
    catch (...) { LOG_ERROR << "unknown exception"; }
    callback(response::fail(k500InternalServerError, "服务器错误"));
    co_return;
}
