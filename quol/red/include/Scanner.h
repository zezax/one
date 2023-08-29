/* Scanner.h - regular expression scanner/tokenizer class header

   Scanner provides a stream of Token objects to Parser.  The
   class is initialized with a string view as input.  Repeated
   calls to scanNext() return Token objects, ending with a Token
   of type tEnd.

   The Token has a number of fields.  Two should always be valid:

   type_ : a value from TokEnum below
   pos_  : approximate origin location in the input

   The following fields have values for specific types:

   min_       : tClosure : minimum repetitions
   max_       : tClosure : maximum repetitions, or -1 for infinity
   flags_     : tFlags   : communicates fIgnoreCase to parser
   multiChar_ : tChars   : represents one or more characters

   The remaining types have meanings on their own:

   tError : zero value, should never occur
   tEnd   : indicates end of stream
   tUnion : corresponds to vertical bar (|) in regex
   tLeft  : open parenthesis for nested regex
   tRight : close parenthesis

   For technical reasons, a union could not be used.

   Usage is like:

   Scanner sc(".*foo(bar)?\\i");
   for (;;) {
     Token tok = sc.scanNext();
     if (tok.type_ == tEnd)
       break;
     handle(tok);
   }

   Scanner will throw RedExcept exceptions for syntax errors.
 */

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


struct Token {
  Token(TokEnum type, size_t pos) : type_(type), pos_(pos) {}

  Token(size_t pos, int min, int max)
    : type_(tClosure), pos_(pos), min_(min), max_(max) {}

  Token(const FlagsTag &, Flags f, size_t pos)
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
