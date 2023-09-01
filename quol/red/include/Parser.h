/* Parser.h - regular expression parser header

   This class parses one or more regular expressions.  The resulting
   form is a single non-deterministic finite automaton (NFA) that
   represents all the added regular expressions.  This NFA can then
   be passed to PowersetConverter which will build an equivalent
   deterministic finite automaton (DFA).

   There are multiple "add" methods here.  The basic add() is the raw
   version.  The more clever addAuto() tries to do what many people
   expect from using regexes in other contexts.  See comments below.
   To use shell-style globs instead of regexes, call addGlob().
   Each add supplies a Result, which must be positive.  Different
   result values can be used to distinguish which regex matched.

   Parsing is done via recursive descent.  A Budget pointer passed to
   the constructor can specify a recursion limit, as well as a limit
   on total automaton states.  A CompStats pointer can also be passed
   in order to get parsing metrics.

   Regular expression syntax and grammar are described in doc/Usage.md

   Usage can be like:

   Parser p(budget, stats);
   p.add("foo", 1, fIgnoreCase);
   p.addAuto("bar\\i", 2, 0);
   p.finish();

   Parser will throw RedExcept exceptions for errors in grammar or usage.
 */

#pragma once

#include <string_view>

#include "Scanner.h"
#include "Budget.h"
#include "Nfa.h"

namespace zezax::red {

class Parser {
public:
  explicit Parser(Budget *budget = nullptr, CompStats *stats = nullptr);
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

  // As add(), but takes shell-style globs instead of full-blown
  // regular expressions.  The only special things in a glob are:
  //   ?      - matches any one character
  //   *      - matches zero or more of any characters
  //   [abc]  - matches any of 'a', 'b', or 'c'
  //   [a-z]  - matches any character in the range 'a' through 'z'
  //   [!a-z] - inverts the character class; ^ or ! both work
  void addGlob(std::string_view glob, Result result, Flags flags);

  // Must call this after all adds, before conversion to DFA.
  void finish();

  void freeAll(); // free parsed nfa

  NfaObj &getNfa() { return nfa_; }
  NfaId getInitial() { return nfa_.getInitial(); }

  Budget    *getBudget() const { return budget_; }
  CompStats *getStats()  const { return stats_; }

private:
  NfaId parseExpr();
  NfaId parsePart();
  NfaId parseMulti();
  NfaId parseUnit();
  NfaId parseCharBits();
  NfaId parseGlob(const Byte *beg, const Byte *end, size_t &tokens);
  NfaId parseClass(const Byte *&ptr, const Byte *beg, const Byte *end);

  Flags      flags_;
  int        level_;
  bool       begun_;
  Token      tok_;
  Scanner    scanner_;
  NfaObj     nfa_;
  Budget    *budget_;
  CompStats *stats_;
};

} // namespace zezax::red
