// dfa executable header

#pragma once

#include <string>
#include <string_view>

#include "Defs.h"
#include "Serializer.h"

namespace zezax::red {


// tag classes...
struct CopyBuf {};
constexpr CopyBuf gCopyBuf;

struct StealNew {};
constexpr StealNew gStealNew;

struct StealMalloc {};
constexpr StealMalloc gStealMalloc;


class Executable {
public:
  Executable() = delete;
  Executable(Executable &&other);

  // these take a serialized dfa...
  Executable(std::string &&buf); // move
  Executable(const std::string &buf); // copy
  Executable(std::string_view sv); // copy
  Executable(const CopyBuf &, const void *ptr, size_t len); // copy
  Executable(const StealNew &, const void *ptr, size_t len); // will delete[]
  Executable(const StealMalloc &, const void *ptr, size_t len); // will free()

  // load dfa from file
  Executable(const char *path);

  ~Executable();

  Executable &operator=(Executable &&rhs);

  // no copying these potentially huge objects
  Executable(const Executable &) = delete;
  Executable &operator=(const Executable &) = delete;

  Result match4(std::string_view sv) const;

private:
  void validate();

  std::string  str_; // storage if needed
  const char  *buf_;
  const char  *end_;
  const Byte  *equivMap_;
  const char  *base_;
  bool         inStr_;
  bool         usedMalloc_;
};

} // namespace zezax::red
