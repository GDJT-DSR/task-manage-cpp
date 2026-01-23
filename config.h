// #pragma once
#ifndef CONFIG_H
#define CONFIG_H

#include <chrono>

namespace config
{
    const std::chrono::duration ACCESS_TOKEN_EXPIRED = std::chrono::hours{1};
    const std::chrono::duration REFRESH_TOKEN_EXPIRED = std::chrono::hours{24};
    const std::chrono::duration JWT_KEY_EXPIRED = std::chrono::hours{6};
} // namespace config

#endif