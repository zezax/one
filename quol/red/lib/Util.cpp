#include "Util.h"

namespace zezax::red {

char fromHexDigit(Byte c) {
  if ((c >= '0') && (c <= '9'))
    return (static_cast<char>(c) - '0');
  if ((c >= 'A') && (c <= 'F'))
    return (static_cast<char>(c) - 'A' + 10);
  if ((c >= 'a') && (c <= 'f'))
    return (static_cast<char>(c) - 'a' + 10);
  return -1;
}

} // namespace zezax::red
