// regular expression non-deterministic finite automaton parser header

#pragma once

#include <string_view>

#include "Scanner.h"
#include "Nfa.h"

namespace zezax::red {

class Parser {
public:
  explicit Parser(CompStats *stats = nullptr);
  ~Parser() = default;

  // Adds a regular expression to the automaton, to yield the specified
  // positive result.  The following flags are honored:
  //   fIgnoreCase - treats upper and lower case letters as equivalent
  //   fLooseStart - effectively prefixes regex with .*
  //   fLooseEnd   - effectively suffixes regex with .*
  // The characters ^ and $ are matched as ordinary characters.
  void add(std::string_view regex, Result result, Flags flags);

  // As above, but this method implements a heuristic for caret and dollar.
  // Both fLooseStart and fLooseEnd are enabled by default.  Then,
  // leading .* is removed.  Similarly, trailing .* is removed.  If a
  // leading ^ is present, it is removed and fLooseStart is disabled.
  // A trailing $ disables fLooseEnd and is removed.
  // Note that this is not correct for something like ^a|b$
  // Also note that ^ and $ are not sepcial characters anywhere else.
  void addAuto(std::string_view regex, Result result, Flags flags);

  // Must call this after all adds, before conversion to DFA.
  void finish();

  void freeAll(); // free parsed nfa

  NfaObj &getNfa() { return nfa_; }
  NfaId getInitial() { return nfa_.getInitial(); }

private:
  NfaId parseExpr();
  NfaId parsePart();
  NfaId parseMulti();
  NfaId parseUnit();
  NfaId parseCharBits();

  Flags      flags_;
  int        level_;
  bool       begun_;
  Token      tok_;
  Scanner    scanner_;
  NfaObj     nfa_;
  CompStats *stats_;
};

} // namespace zezax::red
