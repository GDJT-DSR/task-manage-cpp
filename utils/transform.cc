#include "transform.h"

#include <json/value.h>
#include <json/reader.h>
#include <trantor/utils/Logger.h>

Json::Value transform::string2json(const std::string& str)
{
    Json::Value root;
    Json::CharReaderBuilder builder;
    std::string errors;
    std::unique_ptr<Json::CharReader> reader{builder.newCharReader()};
    bool state = reader->parse(
        str.c_str(),
        str.c_str() + str.length(),
        &root,
        &errors
    );
    if (state) { return root; }
    // LOG_ERROR << errors;
    throw std::runtime_error(errors);
}
