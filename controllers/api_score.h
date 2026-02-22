#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace api
{
    class score : public drogon::HttpController<score>
    {
    public:
        METHOD_LIST_BEGIN
            METHOD_ADD(score::getAll, "", Get);
            METHOD_ADD(score::getSingle, "/{id}", Get);
        METHOD_LIST_END

        static Task<> getAll(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback);
        static Task<> getSingle(HttpRequestPtr req, std::function<void (const HttpResponsePtr&)> callback, int64_t id);
    };
}
