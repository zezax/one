// compilation conveniences header

#pragma once

#include "Parser.h"
#include "Serializer.h"
#include "Executable.h"

namespace zezax::red {

Executable compile(Parser    &rp,
                   Format     fmt   = fmtDirectAuto,
                   CompStats *stats = nullptr);

std::string compileToSerialized(Parser    &rp,
                                Format     fmt   = fmtDirectAuto,
                                CompStats *stats = nullptr);

} // namespace zezax::red
