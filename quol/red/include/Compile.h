// compilation conveniences header

#pragma once

#include <memory>

#include "ReParser.h"
#include "Serializer.h"
#include "Exec.h"

namespace zezax::red {

Executable compile(ReParser  &rp,
                   Format     fmt = fmtDirectAuto,
                   CompStats *stats = nullptr);

std::shared_ptr<const Executable> compileShared(
    ReParser  &rp,
    Format     fmt = fmtDirectAuto,
    CompStats *stats = nullptr);

} // namespace zezax::red
