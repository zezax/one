// re2 behavior demonstration

#include <iostream>
#include <vector>

#include <re2/set.h>

#include "Util.h"

using namespace re2;
using namespace zezax::red;

using std::string;
using std::string_view;
using std::vector;

int main(int argc, char **argv) {
  RE2::Options opt(RE2::Latin1);
  opt.set_max_mem(8L * 1024 * 1024 * 1024);
  opt.set_dot_nl(true);
  opt.set_case_sensitive(false);

  for (int ii = 1; ii < argc; ++ii) {
    string_view arg = argv[ii];
    if (arg == "-cs")
      opt.set_case_sensitive(true);
  }

  try {
    string r = "(([0-9]+)|([0-9]+ [a-z]+))";
    RE2 re(r, opt);
    if (re.ok())
      std::cout << "Regex: " << r << std::endl;
    else
      throw std::runtime_error("failed to create regex");

    string s = "0123456789 foobar";
    StringPiece o;
    bool ok = RE2::FullMatch(s, re, &o);
    std::cout << "FullMatch " << s << ' ' << ok << ' ' << o << std::endl;

    s = "0123456789 foobar.";
    ok = RE2::FullMatch(s, re, &o);
    std::cout << "FullMatch " << s << ' ' << ok << ' ' << o << std::endl;

    ok = RE2::PartialMatch(s, re, &o);
    std::cout << "PartialMatch " << s << ' ' << ok << ' ' << o << std::endl;

    s = "_,.0123456789 foobar.";
    ok = RE2::PartialMatch(s, re, &o);
    std::cout << "PartialMatch " << s << ' ' << ok << ' ' << o << std::endl;

    StringPiece sp = s;
    ok = RE2::Consume(&sp, re, &o);
    std::cout << "Consume " << sp << ' ' << ok << ' ' << o << std::endl;

    s = "0123456789 foobar.";
    sp = s;
    ok = RE2::Consume(&sp, re, &o);
    std::cout << "Consume " << sp << ' ' << ok << ' ' << o << std::endl;

    sp = s;
    ok = RE2::FindAndConsume(&sp, re, &o);
    std::cout << "FindAndConsume " << sp << ' ' << ok << ' ' << o << std::endl;

    s = "_,.0123456789 foobar.";
    sp = s;
    ok = RE2::FindAndConsume(&sp, re, &o);
    std::cout << "FindAndConsume " << sp << ' ' << ok << ' ' << o << std::endl;

    ok = RE2::Replace(&s, re, "()");
    std::cout << "Replace " << s << ' ' << ok << std::endl;

    s = "_,.0123 456 789 foobar";
    ok = RE2::Replace(&s, re, "()");
    std::cout << "Replace " << s << ' ' << ok << std::endl;

    s = "_,.0123 456 789 foobar";
    ok = RE2::GlobalReplace(&s, re, "()");
    std::cout << "GlobalReplace " << s << ' ' << ok << std::endl;

    RE2::Set rset(opt, RE2::ANCHOR_START);
    rset.Add("0", nullptr);      // 0
    rset.Add("0123", nullptr);   // 1
    rset.Add("[0-2]+", nullptr); // 2
    rset.Add("[3-9]+", nullptr); // 3
    rset.Add("012345", nullptr); // 4
    rset.Compile();
    s = "0123456789";
    vector<int> v;
    ok = rset.Match(s, &v, nullptr);
    for (int i : v)
      std::cout << "Set::Match " << s << ' ' << i << std::endl;

    return 0;
  }
  catch (const std::exception &ex) {
    std::cerr << ex.what() << std::endl;
    return 1;
  }
}
