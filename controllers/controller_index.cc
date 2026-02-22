#include "controller_index.h"

#include "utils/response.h"

using namespace controller;
using namespace drogon;

// Add definition of your processing function here
void index::home(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback)
{
    // callback(HttpResponse::newFileResponse("./html/index.html"));
    callback(response::success(req->getPath()));
}
