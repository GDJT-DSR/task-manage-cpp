#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;

namespace user {
    class authorize : public HttpFilter<authorize> {
    public:
        authorize() = default;

        void doFilter(const HttpRequestPtr &req,
                      FilterCallback &&fcb,
                      FilterChainCallback &&fccb) override;
    };
}
