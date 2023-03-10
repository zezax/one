// Main public header for RED - Regular Expression DFA

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "Outcome.h"
#include "Exec.h"

namespace zezax::red {

class Parser;

/*
  The 'Match' functions below return an Outcome, which contains some
  position information.  If this is not needed, the 'Check' functions are a
  more efficient way to get a Result without any positions.
 */

class Red {
public:
  // these are meant for implicit conversion...
  Red(const char *regex);
  Red(const std::string &regex);
  Red(std::string_view regex);
  Red(std::string_view regex, Flags flags);
  Red(Parser &parser);

  // entire text matches re
  Outcome fullMatch(std::string_view text) const;

  // start of text matches re
  Outcome prefixMatch(std::string_view text) const;

  // any subset of text matches re
  Outcome partialMatch(std::string_view text) const;

  // like RE2::Consume()...
  Result prefixConsume(std::string_view &text) const;

  // like RE2::FindAndConsume()...
  Result partialConsume(std::string_view &text) const;

  // sort of like RE2::Set::Match()...
  bool allMatches(std::string_view text, std::vector<Outcome> *out);

  // for easy one-liners...
  static Outcome fullMatch(std::string_view text, const Red &re);
  static Outcome prefixMatch(std::string_view text, const Red &re);
  static Outcome partialMatch(std::string_view text, const Red &re);

  // FIXME: const char *text
  // FIXME: bool test() or Result result() : bool can be styFirst

private:
  Executable program_;
};

} // namespace zezax::red
