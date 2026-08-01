//
// Created by dsr on 2026/7/31.
//
#ifndef TASKCPP_DATE_H
#define TASKCPP_DATE_H
#include <string>

#include "three_party/date.h"

namespace d_utils
{
    date::sys_seconds parseFromDbString(const std::string& str);
} // d_utils

#endif //TASKCPP_DATE_H
