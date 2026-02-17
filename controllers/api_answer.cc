#include "api_answer.h"

#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"

using namespace api;
using namespace permission;

Task<> answer::submit(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int64_t id,
                      std::optional<dto::submit> data)
{
    if (!data)
    {
        callback(response::fail(k400BadRequest, "参数错误"));
        co_return;
    }

    const std::string& userid = req->getParameter("id");
    if (userid.empty())
    {
        callback(response::fail(k401Unauthorized, "获取用户失败"));
        co_return;
    }

    const auto user_permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!has_permission(user_permission, permission::user_permission::submit | permission::user_permission::view_page))
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

    try
    {
        const auto& answer_res = co_await client->execSqlCoro(
            "SELECT a.id, a.question_id, a.answerer_id FROM answers a WHERE a.id = $1",
            std::to_string(id)
        );
        if (answer_res.empty())
        {
            callback(response::fail(k404NotFound, "答案不存在"));
            co_return;
        }

        const auto& answer = answer_res[0];
        const int64_t answerer_id = answer["answerer_id"].as<int64_t>();
        const int64_t question_id = answer["question_id"].as<int64_t>();

        if (answerer_id != std::stoll(userid))
        {
            callback(response::fail(k403Forbidden, "无权修改此答案"));
            co_return;
        }

        const auto& page_res = co_await client->execSqlCoro(
            "SELECT p.id, p.state FROM pages p "
            "JOIN questions q ON q.page_id = p.id "
            "WHERE q.id = $1",
            std::to_string(question_id)
        );
        if (page_res.empty())
        {
            callback(response::fail(k404NotFound, "页面不存在"));
            co_return;
        }

        const auto& page = page_res[0];
        const int64_t page_id = page["id"].as<int64_t>();
        const int64_t page_state = page["state"].as<int64_t>();

        if (!has_permission(page_state, enable | submittable))
        {
            callback(response::fail(k403Forbidden, "页面不可提交"));
            co_return;
        }

        const auto& user_page_res = co_await client->execSqlCoro(
            "SELECT 1 FROM user_page WHERE user_id = $1 AND page_id = $2",
            userid,
            std::to_string(page_id)
        );
        if (user_page_res.empty())
        {
            callback(response::fail(k403Forbidden, "无权访问此页面"));
            co_return;
        }

        co_await client->execSqlCoro(
            "UPDATE answers SET content = $1 WHERE id = $2",
            data->content,
            std::to_string(id)
        );

        callback(response::success());
        co_return;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (...) { LOG_ERROR << "error occurred when submitting answer"; }
    callback(response::fail(k500InternalServerError, "服务器错误"));
    co_return;
}

Task<> answer::upload(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int64_t id)
{
    MultiPartParser parser;
    if (parser.parse(req) == 0 || parser.getFiles().empty())
    {
        callback(response::fail(k400BadRequest, "请传入文件"));
        co_return;
    }
    const auto& file = parser.getFiles().front();
}
