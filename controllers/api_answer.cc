#include "api_answer.h"

#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"

using namespace api;
using namespace permission;

std::string validate(const orm::Row& question, const std::string& expected_type)
{
    const auto state = question["state"].as<int64_t>();
    const auto type = question["type"].as<std::string>();
    const auto start_at_str = question["start_at"].c_str();
    const auto end_at_str = question["end_at"].c_str();
    const auto settings = question["settings"].as<Json::Value>();
    if (!has_permission(state, submit | view_page)) { return "权限不足"; }
    if (type != expected_type) { return "题目类型不匹配"; }
    auto now = trantor::Date::now();
    if (start_at_str && *start_at_str)
    {
        if (auto start_at = trantor::Date::fromDbString(start_at_str);
            start_at > now) { return "页面未开始"; }
    }
    if (end_at_str && *end_at_str)
    {
        if (auto end_at = trantor::Date::fromDbString(end_at_str);
            end_at < now) { return "页面已结束"; }
    }

    return "";
}

Task<> answer::text(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback,
                    int64_t question_id,
                    std::optional<dto::submit> data)
{
    const std::string qid = std::to_string(question_id);
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

    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!has_permission(permission, submit | view_page))
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
        const auto questions = co_await client->execSqlCoro(
            "SELECT state,type,start_at,end_at,settings FROM questions "
            "JOIN pages ON pages.id = page_id "
            "WHERE questions.id = $1",
            qid
        );
        if (questions.empty())
        {
            callback(response::fail(k400BadRequest, "找不到题目"));
            co_return;
        }
        const auto& question = questions.front();
        // 验证权限
        const auto str = validate(question, "fill_in");
        if (!str.empty())
        {
            callback(response::fail(k400BadRequest, str));
            co_return;
        }

        // 验证通过，可以提交

        const auto answers = co_await client->execSqlCoro(
            "SELECT id FROM answers WHERE answerer_id = $1 AND question_id = $2;",
            userid,
            qid
        );


        if (answers.empty())
        {
            // 生成新的回答
            const auto res = co_await client->execSqlCoro(
                "INSERT INTO answers (content,  question_id, answerer_id) "
                "VALUES ($1, $2, $3);",
                data->content,
                question_id,
                userid
            );
            if (res.affectedRows() == 0)
            {
                LOG_ERROR << std::format("error occurred when inserting answer.content={} of question{} by user{}",
                                         data->content, qid, userid);
                callback(response::fail(k500InternalServerError, "存储错误"));
                co_return;
            }
        }
        else
        {
            // 更新回答
            const auto res = co_await client->execSqlCoro(
                "UPDATE answers SET content = $1 WHERE id = 2;",
                data->content
            );
            if (res.affectedRows() == 0)
            {
                LOG_ERROR << std::format("error occurred when updating answer.content={} of question{} by user{}",
                                         data->content, qid, userid);
                callback(response::fail(k500InternalServerError, "存储错误"));
                co_return;
            }
        }
        // client->execSqlCoro()

        callback(response::success());
        co_return;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (const orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); }
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

    const std::string& userid = req->getParameter("id");
    if (userid.empty())
    {
        callback(response::fail(k401Unauthorized, "获取用户失败"));
        co_return;
    }

    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!has_permission(permission, user_permission::upload | view_page))
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
            callback(response::fail(k404NotFound, "回答不存在"));
            co_return;
        }

        const auto& answer = answer_res[0];
        const int64_t answerer_id = answer["answerer_id"].as<int64_t>();
        const int64_t question_id = answer["question_id"].as<int64_t>();

        if (answerer_id != std::stoll(userid))
        {
            callback(response::fail(k403Forbidden, "权限不足"));
            co_return;
        }

        const auto& question_res = co_await client->execSqlCoro(
            "SELECT q.id, q.type, q.page_id FROM questions q WHERE q.id = $1",
            std::to_string(question_id)
        );
        if (question_res.empty())
        {
            callback(response::fail(k404NotFound, "题目不存在"));
            co_return;
        }

        const auto& question = question_res[0];
        const std::string question_type = question["type"].as<std::string>();
        const int64_t page_id = question["page_id"].as<int64_t>();

        if (question_type != "upload")
        {
            callback(response::fail(k400BadRequest, "该题目不支持文件上传"));
            co_return;
        }

        const auto& page_res = co_await client->execSqlCoro(
            "SELECT p.id, p.state FROM pages p WHERE p.id = $1",
            std::to_string(page_id)
        );
        if (page_res.empty())
        {
            callback(response::fail(k404NotFound, "页面不存在"));
            co_return;
        }

        const auto& page = page_res[0];
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

        auto& file = parser.getFiles().front();
        const std::string file_name = file.getFileName();
        const std::string file_type(file.getFileExtension());
        const size_t file_size = file.fileLength();

        if (file.save() != 0)
        {
            callback(response::fail(k500InternalServerError, "文件保存失败"));
            co_return;
        }

        const std::string file_path = app().getUploadPath() + "/" + file_name;

        Json::Value details;
        details["file_name"] = file_name;
        details["file_path"] = file_path;
        details["file_size"] = static_cast<Json::UInt64>(file_size);
        details["file_type"] = file_type;

        Json::StreamWriterBuilder builder;
        builder.settings_["emitUTF8"] = true;
        builder.settings_["indentation"] = "";
        const std::string details_json = Json::writeString(builder, details);

        co_await client->execSqlCoro(
            "UPDATE answers SET content = $1, details = $2 WHERE id = $3",
            file_path,
            details_json,
            std::to_string(id)
        );

        Json::Value result;
        result["file_name"] = file_name;
        result["file_path"] = file_path;
        result["file_size"] = static_cast<Json::UInt64>(file_size);
        callback(response::success(result));
        co_return;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (...) { LOG_ERROR << "error occurred when uploading file"; }
    co_return;
}
