// dfa executable implementation

#include "Exec.h"

#include "Except.h"
#include "Fnv.h"
#include "Serializer.h"

namespace zezax::red {

using std::string;
using std::string_view;


Executable::Executable(Executable &&other)
  : str_(std::move(other.str_)),
    buf_(std::exchange(other.buf_, nullptr)),
    end_(std::exchange(other.end_, nullptr)),
    equivMap_(std::exchange(other.equivMap_, nullptr)),
    base_(std::exchange(other.base_, nullptr)),
    inStr_(std::exchange(other.inStr_, true)),
    usedMalloc_(std::exchange(other.usedMalloc_, false)) {}


Executable::Executable(string &&buf)
  : str_(std::move(buf)),
    buf_(nullptr),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(true),
    usedMalloc_(false) {
  if (str_.empty())
    throw RedExcept("Serialized DFA move-string is empty");
  buf_ = str_.data();
  end_ = buf_ + str_.size();
  validate();
}


Executable::Executable(const string &buf)
  : str_(buf),
    buf_(nullptr),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(true),
    usedMalloc_(false) {
  if (str_.empty())
    throw RedExcept("Serialized DFA copy-string is empty");
  buf_ = str_.data();
  end_ = buf_ + str_.size();
  validate();
}


Executable::Executable(string_view sv)
  : str_(sv),
    buf_(nullptr),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(true),
    usedMalloc_(false) {
  if (str_.empty())
    throw RedExcept("Serialized DFA string_view is empty");
  buf_ = str_.data();
  end_ = buf_ + str_.size();
  validate();
}


Executable::Executable(const CopyBuf &, const void *ptr, size_t len)
  : str_(static_cast<const char *>(ptr), len),
    buf_(nullptr),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(true),
    usedMalloc_(false) {
  if (str_.empty())
    throw RedExcept("Serialized DFA copy-ptr is empty");
  buf_ = str_.data();
  end_ = buf_ + str_.size();
  validate();
}


Executable::Executable(const StealNew &, const void *ptr, size_t len)
  : buf_(static_cast<const char *>(ptr)),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(false),
    usedMalloc_(false) {
  if (!buf_)
    throw RedExcept("Serialized DFA new-ptr is empty");
  end_ = buf_ + len;
  validate();
}


Executable::Executable(const StealMalloc &, const void *ptr, size_t len)
  : buf_(static_cast<const char *>(ptr)),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(false),
    usedMalloc_(true) {
  if (!buf_)
    throw RedExcept("Serialized DFA malloc-ptr is empty");
  end_ = buf_ + len;
  validate();
}


Executable::Executable(const char *path)
  : buf_(nullptr),
    end_(nullptr),
    equivMap_(nullptr),
    base_(nullptr),
    inStr_(true),
    usedMalloc_(false) {
  str_ = loadFromFile(path);
  buf_ = str_.data();
  end_ = buf_ + str_.size();
  validate();
}


Executable::~Executable() {
  if (!inStr_) {
    if (usedMalloc_)
      free(const_cast<char *>(buf_));
    else
      delete[] buf_;
  }
  buf_ = nullptr;
  end_ = nullptr;
  equivMap_ = nullptr;
  base_ = nullptr;
}


Executable &Executable::operator=(Executable &&rhs) {
  str_ = std::move(rhs.str_);
  buf_ = std::exchange(rhs.buf_, nullptr);
  end_ = std::exchange(rhs.end_, nullptr);
  equivMap_ = std::exchange(rhs.equivMap_, nullptr);
  base_ = std::exchange(rhs.base_, nullptr);
  inStr_ = std::exchange(rhs.inStr_, true);
  usedMalloc_ = std::exchange(rhs.usedMalloc_, false);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

void Executable::validate() {
  const char *msg = checkHeader(buf_, end_ - buf_);
  if (msg)
    throw RedExcept(msg);
  const FileHeader *hdr = reinterpret_cast<const FileHeader *>(buf_);
  equivMap_ = reinterpret_cast<const Byte *>(hdr->equivMap_);
  base_ = reinterpret_cast<const char *>(hdr->bytes_);
}

} // namespace zezax::red