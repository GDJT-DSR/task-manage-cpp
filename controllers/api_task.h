#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api {
    class task : public drogon::HttpController<task> {
    public:
        METHOD_LIST_BEGIN
            METHOD_ADD(task::getAll, "/all", Get, "user::authorize");
        METHOD_LIST_END

        Task<> getAll(HttpRequestPtr req, std::function<void (const HttpResponsePtr &)> callback) const;
    };
}
