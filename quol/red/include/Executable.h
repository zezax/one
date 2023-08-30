/* Executable.h - DFA executable header

   Executable encapsulates a serialized optimized DFA for use by
   Matcher.  There are various ways to populate an Executable and
   they have different semantics in terms of ownership and disposal.
   This allows flexibility in avoiding copies of DFAs (which can be
   large) if they come from, say, a memory-mapped file, a database
   query, or an old C library.

   Usage can be like:

   Executable proc(std::move(dfsStr));
   Result res = check(prog, "foobar", styFull);

   Executable throws RedExcept if the DFA is null or corrupted.
 */

#pragma once

#include <string>
#include <string_view>

#include "Types.h"
#include "Serializer.h"

namespace zezax::red {

class Executable {
public:
  Executable()
    : buf_(nullptr), end_(nullptr), equivMap_(nullptr), base_(nullptr),
      inStr_(false), usedMalloc_(false) {}
  Executable(Executable &&other);

  // these take a serialized dfa...
  Executable(std::string &&buf);                              // move
  Executable(const CopyTag &, std::string_view sv);           // copy
  Executable(const DeleteTag &, const void *ptr, size_t len); // will delete[]
  Executable(const FreeTag &, const void *ptr, size_t len);   // will free()
  Executable(const UnownedTag &, std::string_view sv);        // no cleanup

  ~Executable();

  Executable &operator=(Executable &&rhs);

  // no copying these potentially huge objects
  Executable(const Executable &) = delete;
  Executable &operator=(const Executable &) = delete;

  std::string_view serialized() const { return std::string_view(buf_, end_); }

  const FileHeader *getHeader() const {
    return reinterpret_cast<const FileHeader *>(buf_);
  }

  const char *getBase() const { return base_; }
  const Byte *getEquivMap() const { return equivMap_; }
  Format getFormat() const { return fmt_; }
  Byte getLeaderLen() const { return leaderLen_; }
  const Byte *getLeader() const { return leader_; }

private:
  void validate();

  std::string  str_; // storage if needed
  const char  *buf_;
  const char  *end_;
  const Byte  *equivMap_;
  const Byte  *leader_;
  const char  *base_;
  Format       fmt_;
  Byte         leaderLen_;
  bool         inStr_;
  bool         usedNew_;
  bool         usedMalloc_;
};

} // namespace zezax::red
