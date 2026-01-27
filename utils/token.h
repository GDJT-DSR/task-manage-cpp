#pragma once

#include <drogon/drogon.h>

namespace token {
    struct UserClaim {
        int64_t id;
        int64_t permission;
    };

    std::pair<std::string, std::string> generateToken(int64_t id, int64_t permission);

    drogon::Task<std::optional<UserClaim> > parseAccessToken(const std::string &token);

    drogon::Task<std::optional<int64_t> > parseRefreshToken(const std::string &token);
} // namespace token
