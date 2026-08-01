#include "api_page.h"

#include <drogon/orm/CoroMapper.h>

#include "utils/add_to_json.h"
#include "utils/date.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"

using namespace api;
using namespace permission;

// 主观题设置解析
void parseSettings(Json::Value& json, const drogon::orm::Row& single, bool showAnswer, bool objective = false)
{
    Json::Value data;
    const auto settings = single["settings"].as<Json::Value>();
    if (settings.empty()) { return; }
    if (showAnswer)
    {
        if (settings.isMember("answer")) { data["answer"] = settings["answer"]; }
        if (settings.isMember("answer_imgs")) { data["answer_imgs"] = settings["answer_imgs"]; }
    }
    if (objective)
    {
        data["choices"] = settings["choices"];
        if (settings.isMember("multiple") && settings["multiple"].isBool())
        {
            data["multiple"] = settings["multiple"].asBool();
        }
        else { data["multiple"] = false; }
    }
    json["settings"] = data;
}


// Add definition of your processing function here
Task<> page::getAll(const HttpRequestPtr req,
                    std::function<void(const HttpResponsePtr&)> callback)
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
        const auto permission =
            transform::bstring2int<int64_t>(req->getParameter("permission"));
        if (!permission::has_permission(permission, login | view_page))
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
        const auto& res = co_await client->execSqlCoro(
            "SELECT pages.id,title,\"desc\",state,start_at,end_at FROM pages "
            "JOIN public.user_page up on pages.id = up.page_id "
            "WHERE up.user_id = $1",
            id);

        Json::Value data(Json::arrayValue);
        for (const auto& item : res)
        {
            const int64_t state = item["state"].as<int64_t>();
            if (!has_permission(state, enable | visible)) { continue; }
            Json::Value single;
            single["id"] = item["id"].as<int>();
            single["title"] = item["title"].as<std::string>();
            single["readable"] = has_permission(state, readable);
            data.append(std::move(single));
        }

        callback(response::success(data));
        co_return;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (...) {}
    callback(response::fail(k500InternalServerError, "server error"));
    co_return;
}

Task<> page::getSingle(const HttpRequestPtr req,
                       std::function<void(const HttpResponsePtr&)> callback,
                       int id)
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
        const auto permission =
            transform::bstring2int<int64_t>(req->getParameter("permission"));
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
            R"(SELECT pages.id,title,"desc",state,start_at,end_at FROM pages )"
            "JOIN public.user_page up on pages.id = up.page_id "
            "WHERE up.user_id = $1 AND pages.id = $2",
            userid, id);
        if (pages.empty())
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }
        const auto& page = pages.front();
        const int64_t state = page["state"].as<int64_t>();
        // 验证状态
        if (!has_permission(state, enable | visible | readable))
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }
        const auto& questionsWithAnswer = co_await client->execSqlCoro(
            R"(SELECT questions.id,title,"desc",type,settings,max_score,)"
            "answers.id as ans_id,"
            "content,details,score,answers.updated_at "
            "from questions LEFT JOIN answers "
            "ON questions.id = answers.question_id AND answers.answerer_id = $2 "
            "WHERE page_id = $1 "
            "ORDER BY index ASC, questions.id ASC;",
            id, userid);
        Json::Value data;
        data["id"] = page["id"].as<int>();
        data["title"] = page["title"].as<std::string>();
        d_utils::add_to_json_if_exist(data, page, "desc");
        data["state"] = state;
        d_utils::add_to_json_if_exist(data, page, "start_at");
        d_utils::add_to_json_if_exist(data, page, "end_at");
        Json::Value arr(Json::arrayValue);
        bool isChangeable = has_permission(state, changeable);
        bool isEnd = false;
        if (const auto endAtField = page["end_at"]; !endAtField.isNull())
        {
            const auto endAtStr = endAtField.c_str();
            const auto endAtTime = d_utils::parseFromDbString(endAtStr);
            const auto now = std::chrono::system_clock::now();
            if (endAtTime < now) { isEnd = true; }
        }

        for (const auto& question : questionsWithAnswer)
        {
            Json::Value single;
            single["id"] = question["id"].as<int>();
            single["title"] = question["title"].as<std::string>();
            d_utils::add_to_json_if_exist(single, question, "desc");
            const auto type = question["type"].as<std::string>();
            single["type"] = type;
            // d_utils::add_to_json_if_exist<Json::Value>(single, question,
            //                                            "settings");
            bool hasAnswer = !question["ans_id"].isNull();
            parseSettings(single, question,
                          (hasAnswer && !isChangeable) || isEnd || (hasAnswer && !question["score"].isNull()),
                          type == "choose");
            // single["index"] = question["index"].as<int>();
            single["max_score"] = question["max_score"].as<int>();
            if (hasAnswer)
            {
                Json::Value answer;
                answer["id"] = question["ans_id"].as<int>();
                answer["updated_at"] = question["updated_at"].as<std::string>();
                d_utils::add_to_json_if_exist(answer, question, "content");
                d_utils::add_to_json_if_exist<int>(answer, question, "score");
                d_utils::add_to_json_if_exist<Json::Value>(answer, question,
                                                           "details");
                single["answer"] = std::move(answer);
            }
            arr.append(std::move(single));
        }
        data["questions"] = arr;
        callback(response::success(data));
        co_return;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    callback(response::fail(k500InternalServerError, "server error"));

    co_return;
}
