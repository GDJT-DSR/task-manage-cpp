#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;
namespace user
{

  class authorize : public HttpCoroFilter<authorize>
  {
  public:
    authorize() {}
    // void doFilter(const HttpRequestPtr &req,
    //               FilterCallback &&fcb,
    //               FilterChainCallback &&fccb) override;
    Task<HttpResponsePtr> doFilter(const HttpRequestPtr &req);
  };

}
