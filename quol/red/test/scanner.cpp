// unit tests for zezax::red::Scanner

#include <vector>

#include <gtest/gtest.h>

#include "Except.h"
#include "Scanner.h"

using namespace zezax::red;

using std::string_view;
using std::vector;

namespace {

void failScan(string_view sv) {
  Scanner sc(sv);
  EXPECT_THROW(sc.scanOne(), RedExcept);
}

void checkKleen(const char *str, int goodMin, int goodMax) {
  Scanner sc(str);
  Token tok = sc.scanOne();
  EXPECT_EQ(tClosure, tok.type_);
  EXPECT_EQ(tok.min_, goodMin);
  EXPECT_EQ(tok.max_, goodMax);
}

MultiChar mkMulti(const char *str) {
  MultiChar rv;
  bool flip = false;
  const Byte *ptr = reinterpret_cast<const Byte *>(str);
  if (*ptr == '^') {
    flip = true;
    ++ptr;
  }
  for (; *ptr; ++ptr) {
    rv.insert(*ptr);
  }
  if (flip) {
    rv.resize(gAlphabetSize);
    rv.flipAll();
  }
  return rv;
}

void checkMulti(const char *input, const char *answer) {
  Scanner sc(input);
  Token tok = sc.scanOne();
  EXPECT_EQ(tChars, tok.type_);
  EXPECT_EQ(tok.multiChar_, mkMulti(answer));
}

} // anonymous

TEST(Scanner, single) {
  bool escape = false;
  Scanner sc("");
  EXPECT_LT(sc.interpretSingleChar(escape), 0);
  sc.init("\\");
  EXPECT_THROW(sc.interpretSingleChar(escape), RedExcept);
  sc.init("\\x0");
  EXPECT_THROW(sc.interpretSingleChar(escape), RedExcept);
  sc.init("\\xx");
  EXPECT_THROW(sc.interpretSingleChar(escape), RedExcept);

  sc.init("a");
  EXPECT_EQ('a', sc.interpretSingleChar(escape));
  sc.init("\\\\");
  EXPECT_EQ('\\', sc.interpretSingleChar(escape));

  sc.init("\\n");
  EXPECT_EQ('\n', sc.interpretSingleChar(escape));
  EXPECT_FALSE(escape);
  sc.init("\\x0a");
  EXPECT_EQ('\n', sc.interpretSingleChar(escape));
  EXPECT_FALSE(escape);

  sc.init("\\d");
  EXPECT_EQ('d', sc.interpretSingleChar(escape));
  EXPECT_TRUE(escape);
}

TEST(Scanner, expansion) {
  Scanner sc("");
  Token tok(tError, 0);
  for (int ii = 0; ii < static_cast<int>(gAlphabetSize); ++ii) {
    tok = sc.doExpansion(ii, false);
    EXPECT_EQ(1, tok.multiChar_.size());
    EXPECT_TRUE(tok.multiChar_.get(ii));
  }

  EXPECT_THROW(sc.doExpansion('1', true), RedExcept);
  EXPECT_THROW(sc.doExpansion('9', true), RedExcept);

  tok = sc.doExpansion('d', true);
  EXPECT_EQ(10, tok.multiChar_.size());
  EXPECT_TRUE(tok.multiChar_.get('0'));
  tok = sc.doExpansion('D', true);
  EXPECT_EQ(246, tok.multiChar_.size());
  EXPECT_FALSE(tok.multiChar_.get('0'));

  tok = sc.doExpansion('s', true);
  EXPECT_EQ(6, tok.multiChar_.size());
  EXPECT_TRUE(tok.multiChar_.get(' '));
  tok = sc.doExpansion('S', true);
  EXPECT_EQ(250, tok.multiChar_.size());
  EXPECT_FALSE(tok.multiChar_.get(' '));

  tok = sc.doExpansion('w', true);
  EXPECT_EQ(63, tok.multiChar_.size());
  EXPECT_TRUE(tok.multiChar_.get('a'));
  tok = sc.doExpansion('W', true);
  EXPECT_EQ(193, tok.multiChar_.size());
  EXPECT_FALSE(tok.multiChar_.get('a'));

  tok = sc.doExpansion('i', true);
  EXPECT_EQ(tFlags, tok.type_);
  EXPECT_EQ(fIgnoreCase, tok.flags_);
}

TEST(Scanner, kleen) {
  failScan("{}");
  checkKleen("{,}", 0, -1);
  checkKleen("{3}", 3, 3);
  checkKleen("{,3}", 0, 3);
  checkKleen("{3,}", 3, -1);
  checkKleen("{1,3}", 1, 3);
  checkKleen("{0,9}", 0, 9);
  checkKleen("{23,45}", 23, 45);
  failScan("{0,0}");
  failScan("{-1,1}");
  failScan("{2,1}");
  failScan("{1,2,}");
}

TEST(Scanner, multi) {
  failScan("[");
  failScan("[]");
  failScan("[\\]");
  checkMulti("[ab]", "ab");
  checkMulti("[^ab]", "^ab");
  checkMulti("[0-9]", "0123456789");
  checkMulti("[^0-9]", "^0123456789");
  checkMulti("[a0-9b]", "0123456789ab");
  checkMulti("[]]", "]");
  checkMulti("[^]]", "^]");
  checkMulti("[]a]", "]a");
  checkMulti("[^]a]", "^]a");
  checkMulti("[-a]", "-a");
  checkMulti("[^-a]", "^-a");
  checkMulti("[a-]", "-a");
  checkMulti("[^a-]", "^-a");
  checkMulti("[]-]", "-]");
  checkMulti("[^]-]", "^-]");
  checkMulti("[]a-]", "-]a");
  checkMulti("[^]a-]", "^-]a");
  checkMulti("[\n]", "\n");
  checkMulti("[^\n]", "^\n");
  checkMulti("[+--]", "+,-");
  checkMulti("[[-\\]]", "[\\]");
  checkMulti("[]\\--/]", "-./]");
  checkMulti("[^\\x00-\\xff]", "");
  checkMulti("[\\d]", "0123456789");
  checkMulti("[\\d\\s_]", "\t\n\v\f\r 0123456789_");
  checkMulti("[^\\D7]", "012345689");
}


TEST(Scanner, except) {
  bool threw = false;
  try {
    Scanner sc("[foo");
    sc.scanOne();
  }
  catch (RedExceptParse &ex) {
    threw = true;
    EXPECT_EQ(4, ex.getPos());
  }
  EXPECT_TRUE(threw);
}


TEST(Scanner, empty) {
  string_view nul;
  Scanner sc(nul);
  Token tok = sc.scanOne();
  EXPECT_EQ(tEnd, tok.type_);
}


TEST(Scanner, smoke) {
  vector<Token> vec;
  Scanner sc("\\i([Aa]lex?)[^]w-]uz{3,7}|here");
  for (;;) {
    Token tok = sc.scanOne();
    TokEnum type = tok.type_;
    vec.emplace_back(std::move(tok));
    if (type <= tEnd)
      break;
  }
  EXPECT_EQ(sc.numTokens(), vec.size());
  ASSERT_EQ(18, vec.size());
  EXPECT_EQ(tFlags,   vec[ 0].type_);
  EXPECT_EQ(tLeft,    vec[ 1].type_);
  EXPECT_EQ(tChars,   vec[ 2].type_);
  EXPECT_EQ(tChars,   vec[ 3].type_);
  EXPECT_EQ(tChars,   vec[ 4].type_);
  EXPECT_EQ(tChars,   vec[ 5].type_);
  EXPECT_EQ(tClosure, vec[ 6].type_);
  EXPECT_EQ(tRight,   vec[ 7].type_);
  EXPECT_EQ(tChars,   vec[ 8].type_);
  EXPECT_EQ(tChars,   vec[ 9].type_);
  EXPECT_EQ(tChars,   vec[10].type_);
  EXPECT_EQ(tClosure, vec[11].type_);
  EXPECT_EQ(tUnion,   vec[12].type_);
  EXPECT_EQ(tChars,   vec[13].type_);
  EXPECT_EQ(tChars,   vec[14].type_);
  EXPECT_EQ(tChars,   vec[15].type_);
  EXPECT_EQ(tChars,   vec[16].type_);
  EXPECT_EQ(tEnd,     vec[17].type_);
}