// Main public implementation for RED - Regular Expression DFA

#include "Red.h"

#include "Parser.h"
#include "Compile.h"
#include "Matcher.h"

namespace zezax::red {

using std::string;
using std::string_view;

Red::Red(const char *regex) {
  Parser p;
  p.add(regex, 1, 0);
  program_ = compile(p);
}


Red::Red(const string &regex) {
  Parser p;
  p.add(regex, 1, 0);
  program_ = compile(p);
}


Red::Red(string_view regex) {
  Parser p;
  p.add(regex, 1, 0);
  program_ = compile(p);
}


Red::Red(Parser &parser) { // for power users
  program_ = compile(parser);
}


Outcome Red::fullMatch(string_view text) const {
  Matcher m(&program_);
  return m.match<styFull>(text);
}


Outcome Red::prefixMatch(string_view text) const {
  Matcher m(&program_);
  return m.match<styLast>(text);
}


Outcome Red::partialMatch(string_view text) const {
  Matcher m(&program_);
  while (!text.empty()) {       // sliding match
    Outcome oc = m.match<styLast>(text);
    if (oc)
      return oc;
    text.remove_prefix(1);
  }
  return Outcome::fail();
}


/* static */ Outcome Red::fullMatch(string_view text, const Red &re) {
  return re.fullMatch(text);
}


/* static */ Outcome Red::prefixMatch(string_view text, const Red &re) {
  return re.prefixMatch(text);
}


/* static */ Outcome Red::partialMatch(string_view text, const Red &re) {
  return re.partialMatch(text);
}


} // namespace zezax::red
