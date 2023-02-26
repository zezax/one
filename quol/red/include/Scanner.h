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
  tUnion   = 5,
  tLeft    = 6,
  tRight   = 7,
};

struct TokFlag {};
constexpr TokFlag gTokFlag;

struct Token {
  Token(TokEnum type, size_t pos) : type_(type), pos_(pos) {}

  Token(size_t pos, int min, int max)
    : type_(tClosure), pos_(pos), min_(min), max_(max) {}

  Token(const TokFlag &, Flags f, size_t pos)
    : type_(tFlags), pos_(pos), flags_(f) {}

  TokEnum   type_;
  size_t    pos_;
  int       min_;
  int       max_;
  Flags     flags_;
  MultiChar multiChar_;
};


class Scanner {
public:
  Scanner();
  explicit Scanner(std::string_view source);

  void init(std::string_view source);
  Token scanNext();

  size_t numTokens() const { return nTok_; }

  // these are public for testing...
  Token scanSet();
  Token scanClosure();
  Token scanOrdinary();
  int interpretSingleChar(bool &escape);
  Token doExpansion(int ch, bool escape);

private:
  size_t pos() const { return ptr_ - beg_; }
  Token makeDot() const;

  const Byte *beg_;
  const Byte *ptr_;
  const Byte *end_;
  size_t      nTok_;
};

} // namespace zezax::red
