//
// Created by dsr on 2026/7/31.
//

#include "date.h"

namespace d_utils
{
    date::sys_seconds parseFromDbString(const char* str)
    {
        std::istringstream ss{str};

        // 使用 date::sys_seconds 接收解析结果，它表示系统时钟的秒级时间点
        date::sys_seconds time;

        // %F 是 "%Y-%m-%d" 的简写，%T 是 "%H:%M:%S" 的简写
        // %Ez 用来解析带冒号的时区偏移，如 +08:00 [citation:10]
        ss >> date::parse("%F %T%Ez", time);
        if (ss.fail()) { throw std::ios_base::failure("Parsing time failed"); }
        return time;
    }
} // d_utils
