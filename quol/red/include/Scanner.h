// regular expression scanner/tokenizer class header

#pragma once

#include <string_view>

#include "Types.h"

namespace zezax::red {

enum TokEnum {
  tError   = 0,
  tEnd     = 1,
  tFlags   = 2,
  tChars   = 3,
  tClosure = 4,
  tBar     = 5,
  tLeft    = 6,
  tRight   = 7,
};

struct TokFlag {};
constexpr TokFlag gTokFlag;

struct Token {
  Token(TokEnum type, size_t pos) : type_(type), pos_(pos) {}

  Token(size_t pos, int min, int max)
  : type_(tClosure), pos_(pos), min_(min), max_(max) {}

  Token(const TokFlag &, FlagsT f, size_t pos)
    : type_(tFlags), pos_(pos), flags_(f) {}

  TokEnum   type_;
  size_t    pos_;
  int       min_;
  int       max_;
  FlagsT    flags_;
  MultiChar multiChar_;
};


class Scanner {
public:
  Scanner();
  Scanner(std::string_view source);

  void init(std::string_view source);
  Token scanOne();

  // these are public for testing...
  Token scanSet();
  Token scanCount();
  Token scanOrdinary();
  int interpretSingleChar(bool &escape);
  Token doExpansion(int ch, bool escape);

private:
  size_t pos() { return ptr_ - beg_; }

  const Byte *beg_;
  const Byte *ptr_;
  const Byte *end_;
};

} // namespace zezax::red
