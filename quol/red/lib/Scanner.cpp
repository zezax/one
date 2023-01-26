// regular expression scanner/tokenizer implementation

#include "Scanner.h"

#include "Except.h"
#include "Util.h"

namespace zezax::red {

using std::string_view;

namespace {

void charToSet(MultiChar &mc, int ch, bool escape) {
  if (!escape) {
    mc.set(ch);
    return;
  }

  switch (ch) {
  case 'd':
    mc.setSpan('0', '9');
    break;
  case 'D': {
    MultiChar inv;
    inv.resize(gAlphabetSize);
    inv.setSpan('0', '9');
    inv.flipAll();
    mc.unionWith(inv);
    break;
  }

  case 's':
    mc.set('\t');
    mc.set('\n');
    mc.set('\v');
    mc.set('\f');
    mc.set('\r');
    mc.set(' ');
    break;
  case 'S': {
    MultiChar inv;
    inv.resize(gAlphabetSize);
    inv.set('\t');
    inv.set('\n');
    inv.set('\v');
    inv.set('\f');
    inv.set('\r');
    inv.set(' ');
    inv.flipAll();
    mc.unionWith(inv);
    break;
  }

  case 'w':
    mc.setSpan('0', '9');
    mc.setSpan('A', 'Z');
    mc.setSpan('a', 'z');
    mc.set('_');
    break;
  case 'W': {
    MultiChar inv;
    inv.resize(gAlphabetSize);
    inv.setSpan('0', '9');
    inv.setSpan('A', 'Z');
    inv.setSpan('a', 'z');
    inv.set('_');
    inv.flipAll();
    mc.unionWith(inv);
    break;
  }

  default:
    mc.set(ch);
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
}


Token Scanner::scanOne() {
  if (ptr_ >= end_)
    return Token(tEnd, pos());

  switch (*ptr_) {
  case '(': ++ptr_; return Token(tLeft, pos());
  case ')': ++ptr_; return Token(tRight, pos());
  case '*': ++ptr_; return Token(pos(), 0, -1);
  case '+': ++ptr_; return Token(pos(), 1, -1);
  case '?': ++ptr_; return Token(pos(), 0, 1);
  case '|': ++ptr_; return Token(tBar, pos());

  case '.': {
    ++ptr_;
    Token rv(tChars, pos());
    rv.multiChar_.resize(gAlphabetSize);
    rv.multiChar_.setAll();
    return rv;
  }

  case '[':
    ++ptr_;
    return scanSet();

  case '{':
    ++ptr_;
    return scanCount();

  default:
    return scanOrdinary();
  }
}


Token Scanner::scanSet() {
  Token rv(tChars, pos());
  int ch;
  int ch2;
  bool invert = false;
  bool escape = false;
  bool escape2 = false;

  if ((ch = interpretSingleChar(escape)) < 0)
    throw RedExceptParse("incomplete character class", pos());
  if (!escape && (ch == '^')) {
    invert = true;
    ch = interpretSingleChar(escape);
    if (ch < 0)
      throw RedExceptParse("incomplete inverted character class", pos());
  }
  rv.multiChar_.clearAll();

  if (!escape && ((ch == '-') || (ch == ']'))) {
    rv.multiChar_.set(ch);
    ch = interpretSingleChar(escape);
    if (ch < 0)
      throw RedExceptParse("unfinished character class", pos());
  }

  while (escape || (ch != ']')) {
    if ((ch2 = interpretSingleChar(escape2)) < 0)
      throw RedExceptParse("unexpected end of character class", pos());
    if (!escape2 && (ch2 == '-')) {
      if ((ch2 = interpretSingleChar(escape2)) < 0)
        throw RedExceptParse("unfinished character class range", pos());
      if (!escape2 && (ch2 == ']')) {
        rv.multiChar_.set(ch);
        rv.multiChar_.set('-');
        ch = ch2;
        escape = escape2;
        continue;
      }
      if (ch > ch2)
        throw RedExceptParse("backward range", pos());
      rv.multiChar_.setSpan(ch, ch2);
      if ((ch = interpretSingleChar(escape)) < 0)
        throw RedExceptParse("incomplete character class range", pos());
    }
    else {
      charToSet(rv.multiChar_, ch, escape);
      ch = ch2;
      escape = escape2;
    }
  }

  if (invert) {
    rv.multiChar_.resize(gAlphabetSize);
    rv.multiChar_.flipAll();
  }
  return rv;
}


Token Scanner::scanCount() {
  Token rv(tClosure, pos());
  rv.min_ = -1;
  rv.max_ = 0;
  bool seen = false;
  Byte ch = 0;

  while (ptr_ < end_) {
    ch = *ptr_++;
    if (ch == '}')
      break;
    else if (ch == ',') {
      if (rv.min_ >= 0)
        throw RedExceptParse("extra comma in count", pos());
      rv.min_ = rv.max_;
      rv.max_ = 0;
      seen = false;
    }
    else if ((ch >= '0') && (ch <= '9')) {
      rv.max_ = (10 * rv.max_) + (ch - '0');
      seen = true;
    }
    else
      throw RedExceptParse("non-digit in count", pos());
  }

  if (ch != '}')
    throw RedExceptParse("unclosed brace count", pos());
  if ((rv.min_ < 0) && (rv.max_ == 0))
    throw RedExceptParse("illegal count", pos());
  if (!seen)
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

    switch (ch) {
      case '0':  return '\0';
      case 'a':  return '\a';
      case 'b':  return '\b';
      case 'n':  return '\n';
      case 'r':  return '\r';
      case 't':  return '\t';
      case 'v':  return '\v';
      case '\\': return '\\';

      case 'x': {
        if (ptr_ > (end_ - 2))
          throw RedExceptParse("unterminated hex escape", pos());
        char high;
        char low;
        if (((high = fromHexDigit(ptr_[0])) >= 0) &&
            ((low = fromHexDigit(ptr_[1])) >= 0)) {
          ptr_ += 2;
          return (high << 4) | low;
        }
        else
          throw RedExceptParse("invalid hex escape", pos());
        break;
      }

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
  rv.multiChar_.clearAll();

  if (escape) {
    if ((ch >= '1') && (ch <= '9'))
      throw RedExceptParse("backreferences not supported", pos());

    if (ch == 'i') {
      rv.type_ = tFlags;
      rv.flags_ = fIgnoreCase;
      return rv;
    }
  }

  charToSet(rv.multiChar_, ch, escape);
  return rv;
}

} // namespace zezax::red
