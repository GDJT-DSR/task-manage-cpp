#pragma once

#include <drogon/HttpController.h>

using namespace drogon;

namespace controller {
class index : public HttpController<index> {
  public:
    METHOD_LIST_BEGIN
    // use METHOD_ADD to add your custom processing function here;
    // METHOD_ADD(index::get, "/{2}/{1}", Get); // path is
    // /controller/index/{arg2}/{arg1} METHOD_ADD(index::your_method_name,
    // "/{1}/{2}/list", Get); // path is /controller/index/{arg1}/{arg2}/list
    // ADD_METHOD_TO(index::your_method_name, "/absolute/path/{1}/{2}/list",
    // Get); // path is /absolute/path/{arg1}/{arg2}/list

    ADD_METHOD_TO(index::home, "/home", Get);
    ADD_METHOD_TO(index::home, "/home/{}", Get);
    ADD_METHOD_TO(index::home, "/login", Get);

    METHOD_LIST_END

    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req, std::function<void (const
    // HttpResponsePtr &)> &&callback, int p1, std::string p2);
    static void home(const HttpRequestPtr &req,
                     std::function<void(const HttpResponsePtr &)> &&callback);
};
} // namespace controller
