// matching using serialized dfa: unit tests

#include <gtest/gtest.h>

#include "Parser.h"
#include "Compile.h"
#include "Executable.h"
#include "Matcher.h"

using namespace zezax::red;

using std::string;
using std::string_view;
using std::vector;
using testing::TestWithParam;
using testing::Values;


TEST(Matcher, case) {
  Executable rex;
  Executable rexi;
  {
    Parser p;
    Parser pi;
    p.add("abc", 1, 0);
    pi.add("abc", 1, fIgnoreCase);
    rex = compile(p);
    rexi = compile(pi);
  }
  EXPECT_EQ(1, (check<styFull, false>(rex, "abc")));
  EXPECT_EQ(0, (check<styFull, false>(rex, "aBc")));
  EXPECT_EQ(0, (check<styFull, false>(rex, "ABC")));
  EXPECT_EQ(0, (check<styFull, false>(rex, "xyz")));
  EXPECT_EQ(1, (check<styFull, false>(rexi, "abc")));
  EXPECT_EQ(1, (check<styFull, false>(rexi, "aBc")));
  EXPECT_EQ(1, (check<styFull, false>(rexi, "ABC")));
  EXPECT_EQ(0, (check<styFull, false>(rexi, "xyz")));
}


TEST(Matcher, endmarks) {
  Executable rex;
  {
    Parser p;
    p.add("abe", 1, 0);
    p.add("ace", 2, 0);
    rex = compile(p);
  }
  EXPECT_EQ(1, (check<styFull, false>(rex, "abe")));
  EXPECT_EQ(2, (check<styFull, false>(rex, "ace")));
  EXPECT_EQ(0, (check<styFull, false>(rex, "abc")));
  EXPECT_EQ(0, (check<styFull, false>(rex, "age")));
}


TEST(Matcher, start) {
  // degenerate case...
  Executable rex;
  {
    Parser p;
    p.add("a*", 1, 0);
    rex = compile(p);
  }
  Outcome oc = search<styLast, true>(rex, "123a");
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);
  oc = search<styLast, true>(rex, "123az");
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(4, oc.start_); // !!! place where we escape the initial state
  EXPECT_EQ(4, oc.end_);

  // this is the intended usage: one pattern, leading .* follow by non-optional
  {
    Parser p;
    p.add(".*[a-z]+", 1, 0);
    rex = compile(p);
  }
  oc = search<styLast, true>(rex, "123a");
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);
  oc = search<styLast, true>(rex, "123az!");
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_); // now it's what we expect
  EXPECT_EQ(5, oc.end_);
}


TEST(Matcher, startEnd) {
  Executable rex;
  {
    Parser p;
    p.add("[^a]*ab*c",  1, 0);
    p.add("[^d]*dummy", 2, 0);
    rex = compile(p);
  }
  Outcome oc = match(rex, "abbc", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  oc = match(rex, "xabbc", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.start_);
  EXPECT_EQ(5, oc.end_);

  oc = match(rex, "xabbcx", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.start_);
  EXPECT_EQ(5, oc.end_);

  oc = match(rex, "xyzabbcxyz", styLast);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(7, oc.end_);
}

// check & match

TEST(Matcher, matchTangent) {
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+", 1, 0);
    rex = compile(p);
  }
  Outcome oc = match(rex, "0123456789abcdef", styInstant);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(1, oc.end_);
  oc = match(rex, "0123456789abcdef", styFirst);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(10, oc.end_);
  oc = match(rex, "0123456789abcdef", styTangent);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(10, oc.end_);
}


TEST(Matcher, matchLast) {
  Executable rex;
  {
    Parser p;
    p.add("New", 1, fLooseStart);
    p.add("New York", 2, fLooseStart);
    p.add("York", 3, fLooseStart);
    rex = compile(p);
  }
  Outcome oc = match(rex, "I love New York.", styLast);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(7, oc.start_);
  EXPECT_EQ(15, oc.end_);
}


TEST(Matcher, verifyInstant) {
  Style style = styInstant;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = "1";
  Result  rs = check(rex, in, style);
  Outcome oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123abcd";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123abcde";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);
}


TEST(Matcher, verifyFirst) {
  Style style = styFirst;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = "1";
  Result  rs = check(rex, in, style);
  Outcome oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  in = "123abcd";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  in = "123XYZ";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);
}


TEST(Matcher, verifyTangent) {
  Style style = styTangent;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = "1";
  Result  rs = check(rex, in, style);
  Outcome oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  in = "123abcd";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = "123XYZ";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);
}


TEST(Matcher, verifyLast) {
  Style style = styLast;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = "1";
  Result  rs = check(rex, in, style);
  Outcome oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  in = "123abcd";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  in = "123abcde";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);
}


TEST(Matcher, verifyFull) {
  Style style = styFull;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = "1";
  Result  rs = check(rex, in, style);
  Outcome oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(1, oc.end_);

  in = "123";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(3, oc.end_);

  in = "123abcd";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(7, oc.end_);

  in = "123abcde";
  rs = check(rex, in, style);
  oc = match(rex, in, style);
  EXPECT_EQ(oc, search(rex, in, style));
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(0, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(0, oc.end_);
}

// search

TEST(Matcher, searchInstant) {
  Style style = styInstant;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = ".,_1";
  Result rs = scan(rex, in, style);
  Outcome oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123abcd";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123abcde";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);
}


TEST(Matcher, searchFirst) {
  Style style = styFirst;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = ".,_1";
  Result rs = scan(rex, in, style);
  Outcome oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);

  in = ".,_123abcd";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);

  in = ".,_123XYZ";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);
}


TEST(Matcher, searchTangent) {
  Style style = styTangent;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = ".,_1";
  Result rs = scan(rex, in, style);
  Outcome oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);

  in = ".,_123abcd";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(2, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(7, oc.end_);

  in = ".,_123XYZ";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);
}


TEST(Matcher, searchLast) {
  Style style = styLast;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = ".,_1";
  Result rs = scan(rex, in, style);
  Outcome oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);

  in = ".,_123abcd";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(10, oc.end_);

  in = ".,_123abcde";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(10, oc.end_);
}


TEST(Matcher, searchFull) {
  Style style = styFull;
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+a",    2, 0);
    p.add("[0-9]+abcd", 3, 0);
    rex = compile(p);
  }

  string_view in = ".,_1";
  Result rs = scan(rex, in, style);
  Outcome oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(4, oc.end_);

  in = ".,_123";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(1, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(6, oc.end_);

  in = ".,_123abcd";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(3, oc.result_);
  EXPECT_EQ(3, oc.start_);
  EXPECT_EQ(10, oc.end_);

  in = ".,_123abcde";
  rs = scan(rex, in, style);
  oc = search(rex, in, style);
  EXPECT_EQ(rs, oc.result_);
  EXPECT_EQ(0, oc.result_);
  EXPECT_EQ(0, oc.start_);
  EXPECT_EQ(0, oc.end_);
}

// replace

TEST(Matcher, replace) {
  Executable rex;
  {
    Parser p;
    p.add("ab*c", 1, 0);
    rex = compile(p);
  }
  string s;
  EXPECT_EQ(1, replace(rex, "fooac", "bar", s, 9999, styLast));
  EXPECT_EQ("foobar", s);
  EXPECT_EQ(1, replace(rex, string("fooacz"), "bar", s, 9999, styLast));
  EXPECT_EQ("foobarz", s);
  EXPECT_EQ(2, replace(rex, "xacyabbcz", ",", s, 9999, styTangent));
  EXPECT_EQ("x,y,z", s);
}


TEST(Matcher, replaceStyles) {
  Executable rex;
  {
    Parser p;
    p.add("[0-9]+",     1, 0);
    p.add("[0-9]+d",    2, 0);
    p.add("[0-9]+defg", 3, 0);
    rex = compile(p);
  }
  string s;
  EXPECT_EQ(1 , replace(rex, "#123defg!", "xyz", s, 1, styInstant));
  EXPECT_EQ("#xyz23defg!", s);
  EXPECT_EQ(3 , replace(rex, "#123defg!", "xyz", s, 9999, styInstant));
  EXPECT_EQ("#xyzxyzxyzdefg!", s);
  EXPECT_EQ(1, replace(rex, "#123defg!", "xyz", s, 9999, styFirst));
  EXPECT_EQ("#xyzdefg!", s);
  EXPECT_EQ(1, replace(rex, "#123defg!", "xyz", s, 9999, styTangent));
  EXPECT_EQ("#xyzefg!", s);
  EXPECT_EQ(1, replace(rex, "#123defg!", "xyz", s, 9999, styLast));
  EXPECT_EQ("#xyz!", s);
  EXPECT_EQ(0, replace(rex, "#123defg!", "xyz", s, 9999, styFull));
  EXPECT_EQ("#123defg!", s);
  EXPECT_EQ(1, replace(rex, "#123defg", "xyz", s, 9999, styFull));
  EXPECT_EQ("#xyz", s);
}

// matchAll

TEST(Matcher, matchAll) {
  Executable rex;
  {
    Parser p;
    p.add("0",      1, 0);
    p.add("0123",   2, 0);
    p.add("[0-2]+", 3, 0);
    p.add("[3-9]+", 4, 0);
    p.add("012345", 5, 0);
    rex = compile(p);
  }
  vector<Outcome> vec;
  EXPECT_EQ(4, matchAll(rex, "0123456789", vec));
  ASSERT_EQ(4, vec.size());
  EXPECT_EQ(1, vec[0].result_);
  EXPECT_EQ(0, vec[0].start_);
  EXPECT_EQ(1, vec[0].end_);
  EXPECT_EQ(3, vec[1].result_);
  EXPECT_EQ(0, vec[1].start_);
  EXPECT_EQ(3, vec[1].end_);
  EXPECT_EQ(2, vec[2].result_);
  EXPECT_EQ(0, vec[2].start_);
  EXPECT_EQ(4, vec[2].end_);
  EXPECT_EQ(5, vec[3].result_);
  EXPECT_EQ(0, vec[3].start_);
  EXPECT_EQ(6, vec[3].end_);
}


TEST(Matcher, matchAllLoose) {
  Executable rex;
  {
    Parser p;
    p.add("a+", 1, fLooseStart);
    p.add("b+", 2, fLooseStart);
    rex = compile(p);
  }
  vector<Outcome> vec;
  EXPECT_EQ(6, matchAll(rex, ".aa..b.bb..abba.", vec));
  ASSERT_EQ(6, vec.size());
  EXPECT_EQ(1,  vec[0].result_);
  EXPECT_EQ(3,  vec[0].end_);
  EXPECT_EQ(2,  vec[1].result_);
  EXPECT_EQ(6,  vec[1].end_);
  EXPECT_EQ(2,  vec[2].result_);
  EXPECT_EQ(9,  vec[2].end_);
  EXPECT_EQ(1,  vec[3].result_);
  EXPECT_EQ(12, vec[3].end_);
  EXPECT_EQ(2,  vec[4].result_);
  EXPECT_EQ(14, vec[4].end_);
  EXPECT_EQ(1,  vec[5].result_);
  EXPECT_EQ(15, vec[5].end_);
}

// formats

class MatcherTest : public TestWithParam<Format> {};

TEST_P(MatcherTest, check) {
  Format fmt = GetParam();
  Executable rex;
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compile(p, fmt);
  }
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  EXPECT_EQ(0, check(rex, in0, styFull));
  EXPECT_EQ(0, check(rex, in0, 3, styFull));
  EXPECT_EQ(1, check(rex, in1, styFull));
  EXPECT_EQ(2, check(rex, in2, styFull));
}


TEST_P(MatcherTest, match) {
  Format fmt = GetParam();
  Executable rex;
  {
    Parser p;
    p.addAuto("ab*c", 1, 0);
    p.addAuto("ca*b", 2, 0);
    rex = compile(p, fmt);
  }
  const char *in0 = "bca";
  string in1 = "bac";
  string_view in2 = "cab";
  Outcome oc0 = match(rex, in0, styFull);
  Outcome oc00 = match(rex, in0, 3, styFull);
  Outcome oc1 = match(rex, in1, styFull);
  Outcome oc2 = match(rex, in2, styFull);
  EXPECT_EQ(0, oc0.result_);
  EXPECT_EQ(0, oc00.result_);
  EXPECT_EQ(1, oc1.result_);
  EXPECT_EQ(2, oc2.result_);
  EXPECT_EQ(0, oc0.end_);
  EXPECT_EQ(0, oc00.end_);
  EXPECT_EQ(3, oc1.end_);
  EXPECT_EQ(3, oc2.end_);
}


INSTANTIATE_TEST_SUITE_P(A, MatcherTest,
  Values(fmtDirectAuto, fmtDirect1, fmtDirect2, fmtDirect4));
