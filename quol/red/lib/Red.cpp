// Main public implementation for RED - Regular Expression DFA

#include "Red.h"

#include "Parser.h"
#include "Compile.h"
#include "Matcher.h"

namespace zezax::red {

using std::string;
using std::string_view;
using std::vector;

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


Red::Red(string_view regex, Flags flags) {
  Parser p;
  p.add(regex, 1, flags);
  program_ = compile(p);
}


Red::Red(Parser &parser) { // for power users
  program_ = compile(parser);
}


Outcome Red::fullMatch(string_view text) const {
  return match<styFull>(program_, text);
}


Outcome Red::prefixMatch(string_view text) const {
  return match<styLast>(program_, text);
}


Outcome Red::partialMatch(string_view text) const {
  while (!text.empty()) {       // sliding match
    Outcome oc = match<styLast>(program_, text);
    if (oc)
      return oc;
    text.remove_prefix(1);
  }
  return Outcome::fail();
}


// like RE2::Consume()
Result Red::prefixConsume(string_view &text) const {
  Outcome oc = match<styFirst>(program_, text);
  if (oc) {
    text.remove_prefix(oc.end_);
    return oc.result_;
  }
  return 0;
}


// like RE2::FindAndConsume()
Result Red::partialConsume(string_view &text) const {
  Outcome oc = search<styFirst>(program_, text);
  if (oc) {
    text.remove_prefix(oc.end_);
    return oc.result_;
  }
  return 0;
}


bool Red::allMatches(string_view text, vector<Outcome> *out) { // like RE2::Set
  if (out)
    out->clear();
  return matchAll(program_, text, out);
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
