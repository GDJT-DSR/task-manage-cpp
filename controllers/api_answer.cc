#include <ranges>
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
#include <chrono>
#include "utils/date.h"

using namespace api;
using namespace permission;

std::string answer::validate(const orm::Row& question,
                             const std::string& expected_type,
                             bool change)
{
    const auto state = question["state"].as<int64_t>();
    const auto type = question["type"].as<std::string>();
    // const auto settings = question["settings"].as<Json::Value>();
    if (!has_permission(state, submit | view_page)) { return "权限不足"; }
    if (type != expected_type) { return "题目类型不匹配"; }
    auto now = std::chrono::system_clock::now();
    if (auto startAt = question["start_at"]; !startAt.isNull())
    {
        const auto startAtStr = question["start_at"].c_str();
        const auto startAtTime = d_utils::parseFromDbString(startAtStr);
        if (startAtTime > now) { return "页面未开始"; }
    }
    if (auto endAt = question["end_at"]; !endAt.isNull())
    {
        const auto endAtStr = question["end_at"].c_str();
        const auto endAtTime = d_utils::parseFromDbString(endAtStr);
        if (endAtTime < now) { return "页面已结束"; }
    }

    if (change && !has_permission(state, changeable)) { return "禁止更改"; }

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
        const auto answers = co_await client->execSqlCoro(
            "SELECT id, score FROM answers WHERE answerer_id = $1 AND question_id = "
            "$2;",
            userid, question_id);
        const auto& question = questions.front();
        // 验证权限
        const auto str = validate(question, "fill_in", !answers.empty());
        if (!str.empty())
        {
            callback(response::fail(k400BadRequest, str));
            co_return;
        }
        // 判断是否有评分
        if (!answers.empty() && !answers.front()["score"].isNull())
        {
            callback(response::fail(k400BadRequest, "已评分完毕"));
            co_return;
        }

        // 验证通过，可以提交
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
            // 验证可否被更新
            if (!has_permission(question["state"].as<int64_t>(), changeable))
            {
                callback(response::fail(k400BadRequest, "权限不足"));
                co_return;
            }
            // 更新回答
            const auto res = co_await client->execSqlCoro(
                "UPDATE answers SET content = $1 WHERE "
                "id = $2 AND content = $3",
                data->target, answers.front()["id"].c_str(), data->origin);
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
        const auto answers =
            co_await client->execSqlCoro("SELECT id, content, score FROM answers WHERE "
                                         "answerer_id = $1 AND question_id = "
                                         "$2;",
                                         userid, id);
        const auto& question = questions.front();
        // 验证权限
        const auto str = validate(question, "upload", !answers.empty());
        if (!str.empty())
        {
            callback(response::fail(k400BadRequest, str));
            co_return;
        }
        // 判断是否有评分
        if (!answers.empty() && !answers.front()["score"].isNull())
        {
            callback(response::fail(k400BadRequest, "已评分完毕"));
            co_return;
        }
        // 获取文件以待验证
        const auto& file = parser.getFiles().front();
        // 验证后缀名等
        // const auto& settings = question["settings"].as<Json::Value>();
        // const Json::Value& exts = settings["ext"];
        const std::string ext{file.getFileExtension()};
        const auto uuid = utils::getUuid();
        const auto saveName = (uuid + '.' + ext);
        const auto name = uuid + ".webp";
        if (file.saveAs(saveName) != 0)
        {
            co_return callback(
                response::fail(k500InternalServerError, "保存失败"));
        }

        // 图片压缩
        static const std::filesystem::path uploadPath = app().getUploadPath();
        static const std::filesystem::path savePath{"./html/uploads"};
        const std::filesystem::path tempPath = uploadPath / saveName;
        const std::filesystem::path targetPath = savePath / name;
        convertToWebpFile(tempPath, targetPath);

        std::filesystem::remove(tempPath);


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

            // const auto filePath = uploadPath / originFile;

            if (!std::filesystem::remove(savePath / originFile))
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

Task<> answer::choose(HttpRequestPtr req, std::function<void(const HttpResponsePtr&)> callback, int question_id,
                      std::optional<dto::TextSubmit> data)
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
        const auto answers = co_await client->execSqlCoro(
            "SELECT id, score FROM answers WHERE answerer_id = $1 AND question_id = $2;",
            userid, question_id);
        const auto& question = questions.front();

        // 验证权限
        const auto str = validate(question, "choose", !answers.empty());
        if (!str.empty())
        {
            callback(response::fail(k400BadRequest, str));
            co_return;
        }
        // 判断是否有评分
        if (!answers.empty() && !answers.front()["score"].isNull())
        {
            callback(response::fail(k400BadRequest, "已评分完毕"));
            co_return;
        }
        // 验证输入是否合法
        const auto& settings = question["settings"].as<Json::Value>();
        const auto& choices = settings["choices"];
        const bool multiple = settings["multiple"].asBool();
        auto toInt = [](auto&& rng) {
            // 从 subrange 构造 string_view
            std::string_view sv(&*rng.begin(), std::ranges::distance(rng));

            // 去除空格
            auto start = sv.find_first_not_of(' ');
            auto end = sv.find_last_not_of(' ');
            if (start == std::string_view::npos) return 0;
            sv = sv.substr(start, end - start + 1);

            int result = 0;
            const auto r = std::from_chars(sv.data(), sv.data() + sv.size(), result);
            return result;
        };
        if (multiple)
        {
            auto split_view = data->target
                | std::views::split(',')
                | std::views::transform(toInt);
            bool condition = std::ranges::all_of(split_view, [&choices](int num) { return num < choices.size(); });
            if (!condition)
            {
                callback(response::fail(k400BadRequest, "wrong index"));
                co_return;
            }
        }
        else
        {
            const int val = toInt(data->target);
            if (val >= choices.size())
            {
                callback(response::fail(k400BadRequest, "wrong index"));
                co_return;
            }
            data->target = std::to_string(val);
        }

        // 验证通过，可以提交


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
            // 验证可否被更新
            if (question["state"].as<int>() == 2);
            // 更新回答
            const auto res = co_await client->execSqlCoro(
                "UPDATE answers SET content = $1 WHERE "
                "id = $2 AND content = $3",
                data->target, answers.front()["id"].c_str(), data->origin);
            if (res.affectedRows() == 0)
            {
                co_return callback(
                    response::fail(k409Conflict, "数据变化，请刷新重试"));
            }
        }
        co_return callback(response::success());;
    }
    catch (const std::exception& e) { LOG_ERROR << e.what(); }
    catch (const orm::DrogonDbException& e) { LOG_ERROR << e.base().what(); }
    catch (...) { LOG_ERROR << "error occurred when submitting answer"; }
    callback(response::fail(k500InternalServerError, "服务器错误"));
    co_return;
}
