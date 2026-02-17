#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    class page : public drogon::HttpController<page>
    {
    public:
        METHOD_LIST_BEGIN
            METHOD_ADD(page::getAll, "", Get, "user::authorize");
            METHOD_ADD(page::getSingle, "/{id}", Get, "user::authorize");
        METHOD_LIST_END

        static Task<> getAll(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback);

        static Task<> getSingle(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback, int64_t id);
    };
}
