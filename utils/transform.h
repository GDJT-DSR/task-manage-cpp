#pragma once
#include <string>
#include <json/value.h>


namespace transform
{
    template <std::integral IntTp>
    IntTp bstring2int(std::string const& input)
    {
        if (input.size() != sizeof(IntTp))
        {
            throw std::runtime_error("Input string size does not match sizeof(IntTp)");
        }
        IntTp result;
        std::memcpy(&result, input.data(), sizeof(IntTp));
        return result;
    }

    template <std::integral IntTp>
    std::string int2bstring(IntTp input)
    {
        std::string result(sizeof(IntTp), 0);
        std::memcpy(result.data(), &input, sizeof(IntTp));
        return result;
    }

    Json::Value string2json(const std::string&);
} // transform
