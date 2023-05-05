// util.h - utility code

#pragma once

#include <string_view>

namespace zezax::sensorrd {

bool startsWith(std::string_view str, std::string_view head);
bool endsWith(std::string_view str, std::string_view tail);

void split(std::string_view  src,
           char              delim,
           std::string_view &prefix,
           std::string_view &rest);

bool pathExists(const std::string &path);

} // namespace zezax::sensorrd
