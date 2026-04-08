#include "api_answer.h"

#include "drogon/HttpAppFramework.h"
#include "drogon/HttpTypes.h"
#include "drogon/utils/Utilities.h"
#include "trantor/utils/Logger.h"
#include "utils/convert_image.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"
#include <filesystem>
#include <format>

using namespace api;
using namespace permission;

std::string answer::validate(const orm::Row& question,
                             const std::string& expected_type)
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

Task<> answer::text(HttpRequestPtr req,
                    std::function<void(const HttpResponsePtr&)> callback,
                    int question_id, std::optional<dto::TextSubmit> data)
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

    const auto permission =
        transform::bstring2int<int64_t>(req->getParameter("permission"));
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
            "JOIN user_page ON user_page.page_id = pages.id "
            "WHERE questions.id = $1 AND user_id = $2",
            question_id, userid);
        if (questions.empty())
        {
            callback(response::fail(k400BadRequest, "权限不足"));
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
            "SELECT id FROM answers WHERE answerer_id = $1 AND question_id = "
            "$2;",
            userid, question_id);

        if (answers.empty())
        {
            // 生成新的回答
            const auto res = co_await client->execSqlCoro(
                "INSERT INTO answers (content, question_id, answerer_id) "
                "VALUES ($1, $2, $3);",
                data->target, question_id, userid);
            if (res.affectedRows() == 0)
            {
                LOG_ERROR << std::format(
                    "error occurred when inserting "
                    "answer.content={} of question{} by user{}",
                    data->target, question_id, userid);
                callback(response::fail(k500InternalServerError, "存储错误"));
                co_return;
            }
        }
        else
        {
            // 更新回答
            const auto res = co_await client->execSqlCoro(
                "UPDATE answers SET content = $1 WHERE "
                "question_id = $2 AND content = $3",
                data->target, question_id, data->origin);
            if (res.affectedRows() == 0)
            {
                co_return callback(
                    response::fail(k409Conflict, "数据变化，请刷新重试"));
            }
        }
        // client->execSqlCoro()

        co_return callback(response::success());;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (const orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); }
    catch (...) { LOG_ERROR << "error occurred when submitting answer"; }
    callback(response::fail(k500InternalServerError, "服务器错误"));
    co_return;
}

Task<> answer::image(HttpRequestPtr req,
                     std::function<void(const HttpResponsePtr&)> callback,
                     int id)
{
    MultiPartParser parser;
    if (parser.parse(req) != 0 || parser.getFiles().empty())
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

    // 验证用户权限
    const auto permission =
        transform::bstring2int<int64_t>(req->getParameter("permission"));
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
        const auto questions = co_await client->execSqlCoro(
            "SELECT state,type,start_at,end_at,settings FROM questions "
            "JOIN pages ON pages.id = page_id "
            "JOIN user_page ON user_page.page_id = pages.id "
            "WHERE questions.id = $1 AND user_id = $2",
            id, userid);
        if (questions.empty())
        {
            callback(response::fail(k400BadRequest, "权限不足"));
            co_return;
        }
        const auto& question = questions.front();
        // 验证权限
        const auto str = validate(question, "upload");
        if (!str.empty())
        {
            callback(response::fail(k400BadRequest, str));
            co_return;
        }
        // 获取文件以待验证
        const auto& file = parser.getFiles().front();
        // 验证后缀名等
        // const auto& settings = question["settings"].as<Json::Value>();
        // const Json::Value& exts = settings["ext"];
        const std::string ext{file.getFileExtension()};
        const auto uuid = utils::getUuid();
        const auto name = uuid + ".webp";
        if (file.saveAs(uuid) != 0)
        {
            co_return callback(
                response::fail(k500InternalServerError, "保存失败"));
        }

        // 图片压缩
        static const std::filesystem::path uploadPath = app().getUploadPath();
        const std::filesystem::path tempPath = uploadPath / uuid;
        convertToWebpFile(tempPath, "./html/uploads/" + name);
        std::filesystem::remove(tempPath);

        const auto answers =
            co_await client->execSqlCoro("SELECT id,content FROM answers WHERE "
                                         "answerer_id = $1 AND question_id = "
                                         "$2;",
                                         userid, id);

        if (answers.empty())
        {
            // 生成新的回答
            const auto res = co_await client->execSqlCoro(
                "INSERT INTO answers (content, question_id, answerer_id) "
                "VALUES ($1, $2, $3);",
                name, id, userid);
            if (res.affectedRows() == 0)
            {
                LOG_ERROR << std::format(
                    "error occurred when inserting "
                    "answer.content={} of question{} by user{}",
                    name, id, userid);
                callback(response::fail(k500InternalServerError, "存储错误"));
                co_return;
            }
        }
        else
        {
            // 删除原文件
            const auto answer = answers.front();
            const auto originFile = answer["content"].as<std::string>();

            static const std::filesystem::path uploadPath =
                app().getUploadPath();
            const auto filePath = uploadPath / originFile;

            if (!std::filesystem::remove(uploadPath / originFile))
            {
                LOG_ERROR << std::format(
                    "error occurred when removing upload file : {}",
                    originFile);
            }

            // 更新回答
            const auto res = co_await client->execSqlCoro(
                "UPDATE answers SET content = $1 WHERE "
                "id = $2",
                name, answer["id"].c_str());
            if (res.affectedRows() == 0)
            {
                LOG_ERROR << std::format(
                    "error occurred when inserting "
                    "answer.content={} of question{} by user{}",
                    name, id, userid);
                co_return callback(
                    response::fail(k409Conflict, "数据变化，请刷新重试"));
            }
        }

        co_return callback(response::success());
    }

    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (...) { LOG_ERROR << "error occurred when uploading file"; }
    co_return callback(response::fail(k500InternalServerError, "服务器错误"));
}
