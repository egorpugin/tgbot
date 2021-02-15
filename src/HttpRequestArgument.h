#pragma once

#include <string>

namespace tgbot
{

class HttpRequestArgument
{
public:
    std::string name;
    std::string value;

    bool isFile = false;
    std::string fileName;
    std::string mimeType;
};

}
