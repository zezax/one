/* Parser.cpp - regular expression parser implementation

   See general description in Parser.h

   This is a recursive-descent parser that builds an NFA without
   epsilon-transitions.  The parseXYZ() methods implement the grammar.

   Internally, the parser takes advantage of the Scanner class to
   provide a sequence of tokens as input.
 */

#include "Parser.h"

#include <chrono>
#include <limits>

#include "Except.h"

namespace zezax::red {

namespace chrono = std::chrono;

using namespace std::chrono_literals;

using chrono::steady_clock;
using std::numeric_limits;
using std::string_view;

Parser::Parser(Budget *budget, CompStats *stats)
  : flags_(0),
    level_(0),
    begun_(false),
    tok_(tError, gNoPos),
    nfa_(budget),
    budget_(budget),
    stats_(stats) {
  if (stats_) {
    stats_->preNfa_          = steady_clock::now();
    stats_->postNfa_         = steady_clock::time_point(0s);
    stats_->numTokens_       = 0;
    stats_->numPatterns_     = 0;
    stats_->origNfaStates_   = 0;
    stats_->usefulNfaStates_ = 0;
  }
}


void Parser::add(string_view regex, Result result, Flags flags) {
  if (result <= 0)
    throw RedExceptApi("result must be positive");
  if (result > (numeric_limits<Result>::max() -
                static_cast<Result>(gAlphabetSize)))
    throw RedExceptApi("result too large");

  nfa_.setGoal(result);
  flags_ = flags;
  level_ = 0;

  NfaId startWild = gNfaNullId;
  if (flags_ & fLooseStart)
    startWild = nfa_.stateWildcard(); // do early for expected numbering

  scanner_.init(regex);
  tok_ = scanner_.scanNext();

  NfaId state = parseExpr(); // most of the work is here
  if (!state)
    throw RedExceptParse("uncategorized parsing error");
  if (tok_.type_ != tEnd)
    throw RedExceptParse("trailing unparsed input");

  if (flags_ & fIgnoreCase)
    state = nfa_.stateIgnoreCase(state);

  if (startWild)
    state = nfa_.stateConcat(startWild, state);
  if (flags_ & fLooseEnd)
    state = nfa_.stateConcat(state, nfa_.stateWildcard());

  state = nfa_.stateConcat(state, nfa_.stateEndMark(result));
  nfa_.selfUnion(state); // all added regexes are acceptable

  if (stats_) {
    stats_->numTokens_   += scanner_.numTokens();
    stats_->numPatterns_ += 1;
  }
}


void Parser::addAuto(string_view regex, Result result, Flags flags) {
  flags |= fLooseStart | fLooseEnd; // generally expected default

  if (regex.starts_with("\\i")) {
    regex.remove_prefix(2);
    flags |= fIgnoreCase;
  }

  if (regex.starts_with('^')) {
    regex.remove_prefix(1);
    flags &= ~fLooseStart;
  }
  else if (regex.starts_with(".*")) {
    regex.remove_prefix(2);
    flags |= fLooseStart;
  }

  if (regex.ends_with('$')) {
    regex.remove_suffix(1);
    flags &= ~fLooseEnd;
  }
  else if (regex.ends_with(".*")) {
    regex.remove_suffix(2);
    flags |= fLooseEnd;
  }

  add(regex, result, flags);
}


void Parser::addGlob(string_view glob, Result result, Flags flags) {
  if (result <= 0)
    throw RedExceptApi("result must be positive");
  if (result > (numeric_limits<Result>::max() -
                static_cast<Result>(gAlphabetSize)))
    throw RedExceptApi("result too large");

  nfa_.setGoal(result);
  flags_ = flags;

  NfaId startWild = gNfaNullId;
  if (flags_ & fLooseStart)
    startWild = nfa_.stateWildcard(); // do early for expected numbering

  const Byte *beg = reinterpret_cast<const Byte *>(glob.data());
  const Byte *end = beg + glob.size();
  size_t tokens = 0;
  NfaId state = parseGlob(beg, end, tokens);
  if (!state)
    throw RedExceptParse("uncategorized parsing error");

  if (flags_ & fIgnoreCase)
    state = nfa_.stateIgnoreCase(state);

  if (startWild)
    state = nfa_.stateConcat(startWild, state);
  if (flags_ & fLooseEnd)
    state = nfa_.stateConcat(state, nfa_.stateWildcard());

  state = nfa_.stateConcat(state, nfa_.stateEndMark(result));
  nfa_.selfUnion(state); // all added regexes are acceptable

  if (stats_) {
    stats_->numTokens_   += tokens;
    stats_->numPatterns_ += 1;
  }
}


void Parser::addExact(string_view glob, Result result, Flags flags) {
  if (result <= 0)
    throw RedExceptApi("result must be positive");
  if (result > (numeric_limits<Result>::max() -
                static_cast<Result>(gAlphabetSize)))
    throw RedExceptApi("result too large");

  nfa_.setGoal(result);
  flags_ = flags;

  NfaId startWild = gNfaNullId;
  if (flags_ & fLooseStart)
    startWild = nfa_.stateWildcard(); // do early for expected numbering

  NfaId state = gNfaNullId;
  NfaId cur = nfa_.newState(0);

  size_t len = glob.size();
  const Byte *beg = reinterpret_cast<const Byte *>(glob.data());
  const Byte *end = beg + len;
  for (const Byte *ptr = beg; ptr < end; ++ptr) {
    NfaId next = nfa_.newState(0);
    NfaTransition tr;
    tr.next_ = next;
    tr.multiChar_.insert(*ptr);
    nfa_[cur].transitions_.emplace_back(std::move(tr));
    state = nfa_.stateConcat(state, cur);
    cur = next;
  }

  if (flags_ & fLooseEnd)
    nfa_[cur].result_ = result;
  else {
    NfaId next = nfa_.newGoalState();
    NfaTransition tr;
    tr.next_ = next;
    tr.multiChar_.insert(result + gAlphabetSize); // make endmark directly
    nfa_[cur].transitions_.emplace_back(std::move(tr));
  }

  if (flags_ & fIgnoreCase)
    state = nfa_.stateIgnoreCase(state);

  if (startWild)
    state = nfa_.stateConcat(startWild, state);
  if (flags_ & fLooseEnd) {
    state = nfa_.stateConcat(state, nfa_.stateWildcard());
    state = nfa_.stateConcat(state, nfa_.stateEndMark(result));
  }
  nfa_.selfUnion(state); // all added regexes are acceptable

  if (stats_) {
    stats_->numTokens_   += len;
    stats_->numPatterns_ += 1;
  }
}


void Parser::addAs(Language    lang,
                   string_view inp,
                   Result      result,
                   Flags       flags) {
  switch (lang) {
  case langRegexRaw:
    add(inp, result, flags);
    break;
  case langRegexAuto:
    addAuto(inp, result, flags);
    break;
  case langGlob:
    addGlob(inp, result, flags);
    break;
  case langExact:
    addExact(inp, result, flags);
    break;
  default:
    throw RedExceptApi("unrecognized regex language");
  }
}


void Parser::finish() {
  if (nfa_.numStates() == 0)
    nfa_.setInitial(nfa_.newState(1)); // empty matches empty

  // only update stats on first call (just in case)
  if (stats_ && (stats_->postNfa_.time_since_epoch() == 0s))
    stats_->origNfaStates_ = nfa_.activeStates();

  nfa_.dropUselessTransitions();

  if (stats_ && (stats_->postNfa_.time_since_epoch() == 0s)) {
    stats_->usefulNfaStates_ = nfa_.activeStates();
    stats_->postNfa_         = steady_clock::now();
  }
}


void Parser::freeAll() {
  nfa_.freeAll();
}

///////////////////////////////////////////////////////////////////////////////

NfaId Parser::parseExpr() {
  NfaId state = parsePart();
  if (!state)
    return gNfaNullId;

  while (tok_.type_ == tUnion) {
    tok_ = scanner_.scanNext();
    NfaId other = parsePart();
    if (!other)
      return gNfaNullId;
    state = nfa_.stateUnion(state, other);
  }
  return state;
}


NfaId Parser::parsePart() {
  begun_ = false;
  NfaId state = parseMulti();
  if (!state)
    return gNfaNullId;

  while ((tok_.type_ == tChars) || (tok_.type_ == tLeft)) {
    NfaId more = parseMulti();
    if (!more)
      return gNfaNullId;
    state = nfa_.stateConcat(state, more);
  }
  return state;
}


NfaId Parser::parseMulti() {
  NfaId state = parseUnit();
  if (!state)
    return gNfaNullId;

  switch (tok_.type_) {
  case tClosure:
    state = nfa_.stateClosure(state, tok_.min_, tok_.max_);
    break;
  case tFlags:
    flags_ |= tok_.flags_;
    break;
  default:
    return state;
  }

  tok_ = scanner_.scanNext();
  while (tok_.type_ == tFlags) {
    flags_ |= tok_.flags_;
    tok_ = scanner_.scanNext();
  }

  return state;
}


NfaId Parser::parseUnit() {
  while (tok_.type_ == tFlags) {
    flags_ |= tok_.flags_;
    tok_ = scanner_.scanNext();
  }

  NfaId state = gNfaNullId;

  switch (tok_.type_) {
  case tEnd:
    if (begun_)
      throw RedExceptParse("end reached, expected chars or (", tok_.pos_);
    return nfa_.newGoalState(); // empty matches empty
  case tChars:
    begun_ = true;
    state = parseCharBits();
    break;
  case tUnion:
    if (begun_)
      throw RedExceptParse("expected chars or (, got |", tok_.pos_);
    return nfa_.newGoalState(); // empty matches empty
  case tLeft:
    tok_ = scanner_.scanNext();
    if (budget_)
      budget_->takeParens(1);
    ++level_;
    state = parseExpr();
    --level_;
    if (budget_)
      budget_->giveParens(1);
    if (tok_.type_ != tRight)
      throw RedExceptParse("close parenthesis not found", tok_.pos_);
    begun_ = true;
    break;
  case tRight:
    if (begun_ || (level_ <= 0))
      throw RedExceptParse("unexpected )", tok_.pos_);
    return nfa_.newGoalState(); // empty matches empty
  default:
    throw RedExceptParse("expected chars or (", tok_.pos_);
  }

  tok_ = scanner_.scanNext();
  return state;
}


NfaId Parser::parseCharBits() {
  NfaId init = nfa_.newState(0);
  NfaId goal = nfa_.newGoalState();
  nfa_[init].transitions_.emplace_back(NfaTransition{goal, tok_.multiChar_});
  return init;
}

///////////////////////////////////////////////////////////////////////////////

NfaId Parser::parseGlob(const Byte *beg, const Byte *end, size_t &tokens) {
  NfaId state = gNfaNullId;

  for (const Byte *ptr = beg; ptr < end; ++ptr) {
    switch (*ptr) {
    case '*':
      state = nfa_.stateConcat(state, nfa_.stateWildcard());
      break;
    case '?':
      state = nfa_.stateConcat(state, nfa_.stateAnyChar());
      break;
    case '[':
      state = nfa_.stateConcat(state, parseClass(ptr, beg, end));
      break;
    default:
      state = nfa_.stateConcat(state, nfa_.stateChar(*ptr));
      break;
    }
    ++tokens;
  }

  return state;
}


// simplified version of Scanner::scanSet()
NfaId Parser::parseClass(const Byte *&ptr, const Byte *beg, const Byte *end) {
  bool invert = false;
  NfaId init = nfa_.newState(0);
  NfaId goal = nfa_.newGoalState();
  NfaTransition tr;
  tr.next_ = goal;

  if (++ptr >= end)
    throw RedExceptParse("unclosed glob class", ptr - beg);
  Byte xx = *ptr;
  if ((xx == '^') || (xx == '!')) { // two ways to invert in globs
    invert = true;
    if (++ptr >= end)
      throw RedExceptParse("unclosed inverted glob class", ptr - beg);
    xx = *ptr;
  }

  if ((xx == '-') || (xx == ']')) {
    tr.multiChar_.insert(xx);
    if (++ptr >= end)
      throw RedExceptParse("unclosed glob class", ptr - beg);
    xx = *ptr;
  }

  while (xx != ']') {
    if (++ptr >= end)
      throw RedExceptParse("unclosed glob class", ptr - beg);
    Byte yy = *ptr;
    if (yy == '-') {
      if (++ptr >= end)
        throw RedExceptParse("unended glob class range", ptr - beg);
      yy = *ptr;
      if (yy == ']') {
        tr.multiChar_.insert(xx);
        tr.multiChar_.insert('-');
        xx = yy;
        continue;
      }
      if (xx > yy)
        throw RedExceptParse("backward glob class range", ptr - beg);
      tr.multiChar_.setSpan(xx, yy);
      if (++ptr >= end)
        throw RedExceptParse("unclosed glob class range", ptr - beg);
      xx = *ptr;
    }
    else {
      tr.multiChar_.insert(xx);
      xx = yy;
    }
  }

  if (invert) {
    tr.multiChar_.resize(gAlphabetSize);
    tr.multiChar_.flipAll();
  }

  nfa_[init].transitions_.emplace_back(std::move(tr));
  return init;
}

} // namespace zezax::red
