#include "api_task.h"

#include <drogon/orm/CoroMapper.h>

#include "Records.h"
#include "Users.h"
#include "utils/permission.h"
#include "utils/response.h"
#include "utils/transform.h"

using namespace api;
using namespace drogon_model::task;

// Add definition of your processing function here
Task<> task::getAll(const HttpRequestPtr req, std::function<void(const HttpResponsePtr &)> callback) const {
    const std::string &id = req->getParameter("id");
    if (id.empty()) {
        callback(response::fail(k401Unauthorized, "获取用户失败"));
        co_return;
    }

    // 验证permission
    const auto permission = transform::bstring2int<int64_t>(req->getParameter("permission"));
    if (!permission::has_permission(permission, permission::user_permission::login)) {
        callback(response::fail(k403Forbidden, "权限不足"));
        co_return;
    }

    const auto &db = app().getFastDbClient();
    orm::CoroMapper<Users> mapper(db);
    const auto &res = co_await mapper.findBy({Users::Cols::_id, id});

    co_return;
}
