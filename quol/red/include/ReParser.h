// regular expression non-deterministic finite automaton parser header

#pragma once

#include <string_view>

#include "Nfa.h"
#include "Scanner.h"

namespace zezax::red {

class ReParser {
public:
  explicit ReParser(CompStats *stats = nullptr);
  ~ReParser() = default;

  // Adds a regular expression to the automaton, to yield the specified
  // positive result.  The following flags are honored:
  //   fIgnoreCase - treats upper and lower case letters as equivalent
  //   fLooseStart - effectively prefixes regex with .*
  //   fLooseEnd   - effectively suffixes regex with .*
  // The characters ^ and $ are matched as ordinary characters.
  void add(std::string_view regex, Result result, FlagsT flags);

  // As above, but this method implements a heuristic for caret and dollar.
  // Both fLooseStart and fLooseEnd are enabled by default.  Then,
  // leading .* is removed.  Similarly, trailing .* is removed.  If a
  // leading ^ is present, it is removed and fLooseStart is disabled.
  // A trailing $ disables fLooseEnd and is removed.
  // Note that this is not correct for something like ^a|b$
  // Also note that ^ and $ are not sepcial characters anywhere else.
  void addAuto(std::string_view regex, Result result, FlagsT flags);

  void finish();

  void freeAll();

  NfaObj &getNfa() { return obj_; }
  NfaId getNfaInitial() { return obj_.getNfaInitial(); }

private:
  NfaId parseExpr();
  NfaId parsePart();
  NfaId parseMulti();
  NfaId parseUnit();
  NfaId parseCharBits();

  void advance() { tok_ = scanner_.scanOne(); }

  FlagsT         flags_;
  int            level_;
  bool           validated_;
  Token          tok_;
  Scanner        scanner_;
  NfaObj         obj_;
  CompStats     *stats_;
};

} // namespace zezax::red
