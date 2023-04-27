// red behavior demonstration

#include <iostream>
#include <vector>

#include "Red.h"
#include "Parser.h"
#include "Util.h"

using namespace zezax::red;

using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  Flags flags = fIgnoreCase;

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      flags &= ~fIgnoreCase;
  }

  try {
    string r = "(([0-9]+)|([0-9]+ [a-z]+))";
    Red re(r, flags);
    std::cout << "Regex: " << r << std::endl;

    string s = "0123456789 foobar";
    Outcome oc = Red::fullMatch(s, re);
    std::cout << "fullMatch " << s << ' ' << oc.result_
              << ' ' << oc.start_ << ' ' << oc.end_ << std::endl;

    s = "0123456789 foobar.";
    oc = Red::fullMatch(s, re);
    std::cout << "fullMatch " << s << ' ' << oc.result_
              << ' ' << oc.start_ << ' ' << oc.end_ << std::endl;

    oc = Red::partialMatch(s, re);
    std::cout << "partialMatch " << s << ' ' << oc.result_
              << ' ' << oc.start_ << ' ' << oc.end_ << std::endl;

    s = "_,.0123456789 foobar.";
    oc = Red::partialMatch(s, re);
    std::cout << "partialMatch " << s << ' ' << oc.result_
              << ' ' << oc.start_ << ' ' << oc.end_ << std::endl;

    string_view sv = s;
    Result rs = Red::consume(sv, re);
    std::cout << "consume " << sv << ' ' << rs << std::endl;

    s = "0123456789 foobar.";
    sv = s;
    rs = Red::consume(sv, re);
    std::cout << "consume " << sv << ' ' << rs << std::endl;

    sv = s;
    rs = Red::findAndConsume(sv, re);
    std::cout << "findAndConsume " << sv << ' ' << rs << std::endl;

    s = "_,.0123456789 foobar.";
    sv = s;
    rs = Red::findAndConsume(sv, re);
    std::cout << "findAndConsume " << sv << ' ' << rs << std::endl;

    size_t n = Red::replace(&s, re, "()");
    std::cout << "replace " << s << ' ' << n << std::endl;

    s = "_,.0123 456 789 foobar";
    n = Red::replace(&s, re, "()");
    std::cout << "replace " << s << ' ' << n << std::endl;

    s = "_,.0123 456 789 foobar";
    n = Red::globalReplace(&s, re, "()");
    std::cout << "globalReplace " << s << ' ' << n << std::endl;

    Parser p;
    p.add("0",      1, 0);
    p.add("0123",   2, 0);
    p.add("[0-2]+", 3, 0);
    p.add("[3-9]+", 4, 0);
    p.add("012345", 5, 0);
    Red rset(p);
    s = "0123456789";
    vector<Outcome> v;
    n = rset.allMatches(s, &v);
    for (const Outcome &x : v)
      std::cout << "allMatches " << s << ' ' << x.result_ << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
