#pragma once
#include <string>
#include <drogon/orm/Field.h>
#include <drogon/orm/Row.h>
#include <json/value.h>

namespace d_utils
{
    template <typename T = std::string>
    bool add_to_json_if_exist(Json::Value& json, const drogon::orm::Field field, const std::string& c)
    {
        if (!field.isNull())
        {
            json[c] = field.as<T>();
            return true;
        }
        return false;
    }

    template <typename T = std::string>
    bool add_to_json_if_exist(Json::Value& json, const drogon::orm::Row& row, const std::string& c)
    {
        return add_to_json_if_exist<T>(json, row[c], c);
    }
}

