#include "transform.h"

#include <json/value.h>
#include <json/reader.h>

Json::Value transform::string2json(const std::string& str)
{
    Json::Value root;
    Json::CharReaderBuilder reader;
    std::string errors;
    if (std::istringstream iss(str); !Json::parseFromStream(reader, iss, &root, &errors)) { return root; }
    throw std::runtime_error(errors);
}
