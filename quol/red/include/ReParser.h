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
  // leading ^ is removed and fAnchorStart is added to flags.  Similarly,
  // trailing $ is removed and fAnchorEnd is added to flags.  Leading and
  // trailing .* are removed.  When fAnchorStart is not set, the resulting
  // NFA acts like it started with .*.  Similarly for fAnchorEnd.
  // Note that this is not correct for something like ^a|b$
  void add(std::string_view regex, Result result, FlagsT flags);

  // As above with no heuristics.
  void addRaw(std::string_view regex, Result result, FlagsT flags);

  NfaObj &getNfa() { return obj_; }
  NfaState *getNfaInitial() { return obj_.getNfaInitial(); }

private:
  NfaState *parseExpr();
  NfaState *parseTerm();
  NfaState *parseFactor();
  NfaState *parseAtom();
  NfaState *parseCharBits();

  void advance() { tok_ = scanner_.scanOne(); }

  FlagsT         flags_;
  int            level_;
  bool           validated_;
  Token          tok_;
  Scanner        scanner_;
  NfaObj         obj_;
};

} // namespace zezax::red
