// regular expression non-deterministic finite automaton parser header

#pragma once

#include <string_view>

#include "Nfa.h"
#include "Scanner.h"

namespace zezax::red {

class ReParser {
public:
  ReParser();
  ~ReParser() = default;

  // This method implements a heuristic for caret and dollar.  Basically
  // leading .* is removed and fLooseStart is added to flags.  Similarly,
  // trailing .* is removed and fLooseEnd is added to flags.  Leading ^ and
  // trailing $ are removed.  When fLooseStart is set, the resulting
  // NFA acts like it started with .*.  Similarly for fLooseEnd.
  // Note that this is not correct for something like ^a|b$
  // Also note that ^ and $ are not sepcial characters anywhere else.
  void add(std::string_view regex, Result result, FlagsT flags);

  // As above with no heuristics.
  void addRaw(std::string_view regex, Result result, FlagsT flags);

  void finish();

  void freeAll();

  NfaObj &getNfa() { return obj_; }
  NfaId getNfaInitial() { return obj_.getNfaInitial(); }

private:
  NfaId parseExpr();
  NfaId parseTerm();
  NfaId parseFactor();
  NfaId parseAtom();
  NfaId parseCharBits();

  void advance() { tok_ = scanner_.scanOne(); }

  FlagsT         flags_;
  int            level_;
  bool           validated_;
  Token          tok_;
  Scanner        scanner_;
  NfaObj         obj_;
};

} // namespace zezax::red
