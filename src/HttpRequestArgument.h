#pragma once

#include <string>

namespace tgbot {

struct http_request_argument {
    std::string name;
    std::string value;
    std::string filename;
    std::string mimetype;

    bool is_file() const { return filename.empty(); }
};

}
