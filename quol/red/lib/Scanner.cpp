// regular expression scanner/tokenizer implementation

#include "Scanner.h"

#include "Except.h"
#include "Util.h"

namespace zezax::red {

using std::string_view;

namespace {

void charToSet(MultiChar &mc, int ch, bool escape) {
  if (!escape) {
    mc.insert(ch);
    return;
  }

  MultiChar inv;

  switch (ch) {
  case 'd':
    mc.setSpan('0', '9');
    break;
  case 'D':
    inv.resize(gAlphabetSize);
    inv.setSpan('0', '9');
    inv.flipAll();
    mc.unionWith(inv);
    break;

  case 's':
    mc.insert('\t');
    mc.insert('\n');
    mc.insert('\v');
    mc.insert('\f');
    mc.insert('\r');
    mc.insert(' ');
    break;
  case 'S':
    inv.resize(gAlphabetSize);
    inv.insert('\t');
    inv.insert('\n');
    inv.insert('\v');
    inv.insert('\f');
    inv.insert('\r');
    inv.insert(' ');
    inv.flipAll();
    mc.unionWith(inv);
    break;

  case 'w':
    mc.setSpan('0', '9');
    mc.setSpan('A', 'Z');
    mc.setSpan('a', 'z');
    mc.insert('_');
    break;
  case 'W':
    inv.resize(gAlphabetSize);
    inv.setSpan('0', '9');
    inv.setSpan('A', 'Z');
    inv.setSpan('a', 'z');
    inv.insert('_');
    inv.flipAll();
    mc.unionWith(inv);
    break;

  default:
    mc.insert(ch);
    break;
  }
}

} // anonymous

///////////////////////////////////////////////////////////////////////////////

Scanner::Scanner() {
  string_view nul;
  init(nul);
}


Scanner::Scanner(string_view source) {
  init(source);
}


void Scanner::init(string_view source) {
  beg_ = reinterpret_cast<const Byte *>(source.data());
  ptr_ = beg_;
  end_ = beg_ + source.size();
  nTok_ = 0;
}


Token Scanner::scanNext() {
  ++nTok_;
  if (ptr_ >= end_)
    return Token(tEnd, pos());

  switch (*ptr_) {
  case '(': ++ptr_; return Token(tLeft, pos());
  case ')': ++ptr_; return Token(tRight, pos());
  case '*': ++ptr_; return Token(pos(), 0, -1);
  case '+': ++ptr_; return Token(pos(), 1, -1);
  case '?': ++ptr_; return Token(pos(), 0, 1);
  case '|': ++ptr_; return Token(tUnion, pos());

  case '.':
    ++ptr_;
    return makeDot();

  case '[':
    ++ptr_;
    return scanSet();

  case '{':
    ++ptr_;
    return scanClosure();

  default:
    return scanOrdinary();
  }
}


Token Scanner::scanSet() {
  Token rv(tChars, pos());
  int xx;             // primary char being parsed, start or only char
  int yy;             // secondary char being parsed, end of range
  bool xxEsc = false; // escape status for xx
  bool yyEsc = false; // escape status for yy
  bool invert = false;

  xx = interpretSingleChar(xxEsc);
  if (xx < 0)
    throw RedExceptParse("incomplete character class", pos());
  if (!xxEsc && (xx == '^')) {
    invert = true;
    xx = interpretSingleChar(xxEsc);
    if (xx < 0)
      throw RedExceptParse("incomplete inverted character class", pos());
  }
  rv.multiChar_.truncate();

  if (!xxEsc && ((xx == '-') || (xx == ']'))) {
    rv.multiChar_.insert(xx);
    xx = interpretSingleChar(xxEsc);
    if (xx < 0)
      throw RedExceptParse("unfinished character class", pos());
  }

  while (xxEsc || (xx != ']')) {
    yy = interpretSingleChar(yyEsc);
    if (yy < 0)
      throw RedExceptParse("unexpected end of character class", pos());
    if (!yyEsc && (yy == '-')) {
      yy = interpretSingleChar(yyEsc);
      if (yy < 0)
        throw RedExceptParse("unfinished character class range", pos());
      if (!yyEsc && (yy == ']')) {
        rv.multiChar_.insert(xx);
        rv.multiChar_.insert('-');
        xx = yy;
        xxEsc = yyEsc;
        continue;
      }
      if (xx > yy)
        throw RedExceptParse("backward range", pos());
      rv.multiChar_.setSpan(xx, yy);
      xx = interpretSingleChar(xxEsc);
      if (xx < 0)
        throw RedExceptParse("incomplete character class range", pos());
    }
    else {
      charToSet(rv.multiChar_, xx, xxEsc);
      xx = yy;
      xxEsc = yyEsc;
    }
  }

  if (invert) {
    rv.multiChar_.resize(gAlphabetSize);
    rv.multiChar_.flipAll();
  }
  return rv;
}


Token Scanner::scanClosure() {
  Token rv(tClosure, pos());
  rv.min_ = -1;
  rv.max_ = 0;
  bool haveMax = false;
  Byte ch = 0;

  while (ptr_ < end_) {
    ch = *ptr_++;
    if (ch == '}')
      break;
    if (ch == ',') {
      if (rv.min_ >= 0)
        throw RedExceptParse("extra comma in count", pos());
      rv.min_ = rv.max_;
      rv.max_ = 0;
      haveMax = false;
    }
    else if ((ch >= '0') && (ch <= '9')) {
      rv.max_ = (10 * rv.max_) + (ch - '0');
      haveMax = true;
    }
    else
      throw RedExceptParse("non-digit in count", pos());
  }

  if (ch != '}')
    throw RedExceptParse("unclosed brace count", pos());
  if ((rv.min_ < 0) && (rv.max_ == 0))
    throw RedExceptParse("illegal count", pos());
  if (!haveMax)
    rv.max_ = -1;
  if ((rv.min_ == 0) && (rv.max_ == 0))
    throw RedExceptParse("zero brace count", pos());
  if ((rv.max_ >= 0) && (rv.max_ < rv.min_))
    throw RedExceptParse("backwards brace count", pos());
  if (rv.min_ < 0)
    rv.min_ = rv.max_;
  return rv;
}


Token Scanner::scanOrdinary() {
  bool escape = false;
  int ch = interpretSingleChar(escape);
  if (ch < 0)
    return Token(tEnd, pos());
  return doExpansion(ch, escape);
}


int Scanner::interpretSingleChar(bool &escape) {
  if (ptr_ >= end_)
    return -1;
  Byte ch = *ptr_++;
  escape = false;

  if (ch == '\\') {
    if (ptr_ >= end_)
      throw RedExceptParse("partial backslash escape", pos());
    ch = *ptr_++;

    char high;
    char low;

    switch (ch) {
    case '0':  return '\0';
    case 'a':  return '\a';
    case 'b':  return '\b';
    case 'n':  return '\n';
    case 'r':  return '\r';
    case 't':  return '\t';
    case 'v':  return '\v';
    case '\\': return '\\';

    case 'x':
      if (ptr_ > (end_ - 2))
        throw RedExceptParse("unterminated hex escape", pos());
      if (((high = fromHexDigit(ptr_[0])) >= 0) && // !!! assign-as-condition
          ((low  = fromHexDigit(ptr_[1])) >= 0)) { // !!! assign-as-condition
        ptr_ += 2;
        return (static_cast<unsigned>(high) << 4U) | static_cast<unsigned>(low);
      }
      throw RedExceptParse("invalid hex escape", pos());
      break;

    default:
      escape = true;
      break;
    }
  }
  return ch;
}


Token Scanner::doExpansion(int ch, bool escape) {
  if (ch < 0)
    throw RedExceptParse("trying to expand illegal character", pos());

  Token rv(tChars, pos());
  rv.multiChar_.truncate();

  if (escape) {
    if ((ch >= '1') && (ch <= '9'))
      throw RedExceptParse("dfa does not support backreferences", pos());

    if (ch == 'i') {
      rv.type_ = tFlags;
      rv.flags_ = fIgnoreCase;
      return rv;
    }
  }

  charToSet(rv.multiChar_, ch, escape);
  return rv;
}

///////////////////////////////////////////////////////////////////////////////

Token Scanner::makeDot() const {
  Token rv(tChars, pos());
  rv.multiChar_.resize(gAlphabetSize);
  rv.multiChar_.setAll();
  return rv;
}

} // namespace zezax::red
