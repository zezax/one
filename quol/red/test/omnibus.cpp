// unit tests for regular expressions

#include <gtest/gtest.h>

#include <ostream>

#include "Except.h"
#include "ReParser.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"

using namespace testing;
using namespace zezax::red;

using std::ostream;
using std::string;


// FIXME: move this to a matching test
TEST(Omnibus, multi) {
  ReParser p;
  p.addRaw("a", 1, 0);
  p.addRaw("aa", 2, 0);
  p.addRaw("aaa", 3, 0);
  p.finish();
  DfaObj dfa = convertNfaToDfa(p.getNfa());
  p.freeAll();
  {
    DfaMinimizer dm(dfa);
    dm.minimize();
  }
  EXPECT_EQ(0, dfa.matchWhole(""));
  EXPECT_EQ(0, dfa.matchWhole("0"));
  EXPECT_EQ(1, dfa.matchWhole("a"));
  EXPECT_EQ(2, dfa.matchWhole("aa"));
  EXPECT_EQ(3, dfa.matchWhole("aaa"));
  EXPECT_EQ(0, dfa.matchWhole("aaaa"));
}


struct Rec {
  const char *regex_;
  const char *text_;
  bool match_;
};

ostream &operator<<(ostream &os, const Rec &r) {
  os << r.regex_ << ' ' << r.text_ << ' ' << r.match_;
  return os;
}

class Omnibus : public TestWithParam<Rec> {};

#if 0

TEST_P(Omnibus, parse) {
  Rec r = GetParam();
  try {
    ReParser p;
    p.add(r.regex_, 1, 0);
    p.finish();
    p.freeAll();
    EXPECT_FALSE(r.text_ == nullptr);
  }
  catch (const RedExcept &ex) {
    EXPECT_TRUE(r.text_ == nullptr);
  }
}


TEST_P(Omnibus, convert) {
  Rec r = GetParam();
  try {
    ReParser p;
    p.add(r.regex_, 1, 0);
    p.finish();
    EXPECT_FALSE(r.text_ == nullptr);
    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
  }
  catch (const RedExcept &ex) {
    EXPECT_TRUE(r.text_ == nullptr);
  }
}


TEST_P(Omnibus, minimize) {
  Rec r = GetParam();
  try {
    ReParser p;
    p.add(r.regex_, 1, 0);
    p.finish();
    EXPECT_FALSE(r.text_ == nullptr);
    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
  }
  catch (const RedExcept &ex) {
    EXPECT_TRUE(r.text_ == nullptr);
  }
}

#endif

TEST_P(Omnibus, match) {
  Rec r = GetParam();
  try {
    ReParser p;
    p.add(r.regex_, 1, 0);
    p.finish();
    EXPECT_FALSE(r.text_ == nullptr);
    DfaObj dfa = convertNfaToDfa(p.getNfa());
    p.freeAll();
    {
      DfaMinimizer dm(dfa);
      dm.minimize();
    }
    Result res = dfa.match(r.text_);
    EXPECT_EQ(r.match_, (res == 1));
  }
  catch (const RedExcept &ex) {
    EXPECT_TRUE(r.text_ == nullptr);
  }
}


INSTANTIATE_TEST_SUITE_P(A, Omnibus, Values(
Rec{"abc", "abc", true},
Rec{"abc", "xbc", false},
Rec{"abc", "axc", false},
Rec{"abc", "abx", false},
Rec{"abc", "xabcy", true},
Rec{"abc", "ababc", true},
Rec{"ab*c", "abc", true},
Rec{"ab*bc", "abc", true},
Rec{"ab*bc", "abbc", true},
Rec{"ab*bc", "abbbbc", true},
Rec{"ab+bc", "abbc", true},
Rec{"ab+bc", "abc", false},
Rec{"ab+bc", "abq", false},
Rec{"ab+bc", "abbbbc", true},
Rec{"ab?bc", "abbc", true},
Rec{"ab?bc", "abc", true},
Rec{"ab?bc", "abbbbc", false},
Rec{"ab?c", "abc", true},
Rec{"^abc$", "abc", true},
Rec{"^abc$", "abcc", false},
Rec{"^abc", "abcc", true},
Rec{"^abc$", "aabc", false},
Rec{"abc$", "aabc", true},
Rec{"^", "abc", true},
Rec{"$", "abc", true},
Rec{"a.c", "abc", true},
Rec{"a.c", "axc", true},
Rec{"a.*c", "axyzc", true},
Rec{"a.*c", "axyzd", false},
Rec{"a[bc]d", "abc", false},
Rec{"a[bc]d", "abd", true},
Rec{"a[b-d]e", "abd", false},
Rec{"a[b-d]e", "ace", true},
Rec{"a[b-d]", "aac", true},
Rec{"a[-b]", "a-", true},
Rec{"a[b-]", "a-", true},
Rec{"a[b-a]", nullptr, false},
Rec{"a[]b", nullptr, false},
Rec{"a[", nullptr, false},
Rec{"a]", "a]", true},
Rec{"a[]]b", "a]b", true},
Rec{"a[^bc]d", "aed", true},
Rec{"a[^bc]d", "abd", false},
Rec{"a[^-b]c", "adc", true},
Rec{"a[^-b]c", "a-c", false},
Rec{"a[^]b]c", "a]c", false},
Rec{"a[^]b]c", "adc", true},
Rec{"ab|cd", "abc", true},
Rec{"ab|cd", "abcd", true},
Rec{"()ef", "def", true},
Rec{"()*", "", true},
Rec{"*a", nullptr, false},
Rec{"^*", nullptr, false},
Rec{"$*", "$$", true},
Rec{"(*)b", nullptr, false},
Rec{"$b", "b", false},
Rec{"a\\", nullptr, false},
Rec{"a\\(b", "a(b", true},
Rec{"a\\(*b", "ab", true},
Rec{"a\\(*b", "a((b", true},
Rec{"a\\\\b", "a\\b", true},
Rec{"abc)", nullptr, false},
Rec{"(abc", nullptr, false},
Rec{"((a))", "abc", true},
Rec{"(a)b(c)", "abc", true},
Rec{"a+b+c", "aabbabc", true},
Rec{"a**", nullptr, false},
Rec{"a*?", nullptr, false},
Rec{"(a*)*", "aaa", true},
Rec{"(a*)+", "", true},
Rec{"(a|)*", "", true},
Rec{"(a*|b)*", "bbb", true},
Rec{"(a+|b)*", "ab", true},
Rec{"(a+|b)+", "ab", true},
Rec{"(a+|b)?", "ab", true},
Rec{"[^ab]*", "cde", true},
Rec{"(^)*", "", true},
Rec{"(ab|)*", "", true},
Rec{")(", nullptr, false},
Rec{"", "abc", true},
Rec{"abc", "", false},
Rec{"a*", "", true},
Rec{"([abc])*d", "abbbcd", true},
Rec{"([abc])*bcd", "abcd", true},
Rec{"a|b|c|d|e", "e", true},
Rec{"(a|b|c|d|e)f", "ef", true},
Rec{"((a*|b))*", "abab", true},
Rec{"abcd*efg", "abcdefg", true},
Rec{"ab*", "xabyabbbz", true},
Rec{"ab*", "xayabbbz", true},
Rec{"(ab|cd)e", "abcde", true},
Rec{"[abhgefdc]ij", "hij", true},
Rec{"^(ab|cd)e", "abcde", false},
Rec{"(abc|)ef", "abcdef", true},
Rec{"(a|b)c*d", "abcd", true},
Rec{"(ab|ab*)bc", "abc", true},
Rec{"a([bc]*)c*", "abc", true},
Rec{"a([bc]*)(c*d)", "abcd", true},
Rec{"a([bc]+)(c*d)", "abcd", true},
Rec{"a([bc]*)(c+d)", "abcd", true},
Rec{"a[bcd]*dcdcde", "adcdcde", true},
Rec{"a[bcd]+dcdcde", "adcdcde", false},
Rec{"(ab|a)b*c", "abc", true},
Rec{"((a)(b)c)(d)", "abcd", true},
Rec{"[a-zA-Z_][a-zA-Z0-9_]*", "alpha", true},
Rec{"^a(bc+|b[eh])g|.h$", "abhg", true}, // changed
Rec{"(bc+d$|ef*g.|h?i(j|k))", "effgz", true},
Rec{"(bc+d$|ef*g.|h?i(j|k))", "ij", true},
Rec{"(bc+d$|ef*g.|h?i(j|k))", "effg", false},
Rec{"(bc+d$|ef*g.|h?i(j|k))", "bcdd", false},
Rec{"(bc+d$|ef*g.|h?i(j|k))", "reffgz", true},
Rec{"((((((((((a))))))))))", "a", true},
Rec{"(((((((((a)))))))))", "a", true},
Rec{"multiple words of text", "uh-uh", false},
Rec{"multiple words", "multiple words, yeah", true},
Rec{"(.*)c(.*)", "abcde", true},
Rec{"\\((.*), (.*)\\)", "(a, b)", true},
Rec{"[ -~]*", "abc", true},
Rec{"[ -~ -~]*", "abc", true},
Rec{"[ -~ -~ -~]*", "abc", true},
Rec{"[ -~ -~ -~ -~]*", "abc", true},
Rec{"[ -~ -~ -~ -~ -~]*", "abc", true},
Rec{"[ -~ -~ -~ -~ -~ -~]*", "abc", true},
Rec{"[ -~ -~ -~ -~ -~ -~ -~]*", "abc", true},
Rec{"[k]", "ab", false},
Rec{"abcd", "abcd", true},
Rec{"a(bc)d", "abcd", true},
Rec{"\\ia", "A", true},
Rec{"a\\i", "A", true},
Rec{"(ab)\\1", nullptr, false},
Rec{"\\x61[\\x00-\\xff]", "ab", true},
Rec{"ab{2,5}c", "abbbc", true},
Rec{"ab{3}c", "abbbc", true},
Rec{"ab{3,}c", "abbbbc", true},
Rec{"ab{,3}c", "abbc", true},
Rec{"ab{}", nullptr, false},
Rec{"ab{", nullptr, false},
Rec{"ab{1", nullptr, false},
Rec{"ab{x,1}c", nullptr, false},
Rec{"ab{1,y}c", nullptr, false},
Rec{"\\w+:[0-9]+", "a simple_host:666 now", true},
Rec{"^([a-zA-Z0-9]|-)+(\\.([a-zA-Z0-9]|-)+)*(\\.)?$", "a-9.9-a.", true},
Rec{"^(((((.)?){100})*)+)$", "anything", true},
Rec{"^[^\\d\\D]$", " ", false},
Rec{"^\\d+$", "0123456789", true},
Rec{"^[0-9a-f+.-]{5,}$", "0abc", false},
Rec{"", "", true},
Rec{"()", "", true},
Rec{"^.*$", "", true},
Rec{"^foo|bar|[A-Z]$", "Z", true},
Rec{"^(a)(b)(c)(d)$", "abcd", true},
Rec{"\\i([wand]{5})", "A fish called *Wanda*", true},
Rec{"^([^\\\\])$", "D", true},
Rec{"^(\\w+)(:(([^;\\\\]|\\\\.)*))?;?",
    "bar:1,0x2F,030,4,5;baz:true;fooby:false,true", true},
Rec{"^a.b$", "a\nb", true},
Rec{"^(((((llx((-3)|(4)))(;(llx((-3)|(4))))*))))$", "llx-3;llx4", true},
Rec{"(|a)*", "aaaaa", true},
Rec{"(|a)+", "aaaaa", true},
Rec{"^[abc]+$", "def", false}
));
