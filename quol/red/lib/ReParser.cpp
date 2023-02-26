// regular expression non-deterministic finite automaton parser implementation
// recursive-descent parser produces nfa without epsilon transitions

#include "ReParser.h"

#include <chrono>

#include "Except.h"

namespace zezax::red {

namespace chrono = std::chrono;

using namespace std::chrono_literals;

using chrono::steady_clock;
using std::string_view;

ReParser::ReParser(CompStats *stats)
  : flags_(0),
    level_(0),
    begun_(false),
    tok_(tError, gNoPos),
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


void ReParser::add(string_view regex, Result result, Flags flags) {
  if (result <= 0)
    throw RedExceptApi("result must be positive");

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


void ReParser::addAuto(string_view regex, Result result, Flags flags) {
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


void ReParser::finish() {
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


void ReParser::freeAll() {
  nfa_.freeAll();
}

///////////////////////////////////////////////////////////////////////////////

NfaId ReParser::parseExpr() {
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


NfaId ReParser::parsePart() {
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


NfaId ReParser::parseMulti() {
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


NfaId ReParser::parseUnit() {
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
    ++level_;
    state = parseExpr();
    --level_;
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


NfaId ReParser::parseCharBits() {
  NfaId init = nfa_.newState(0);
  NfaId goal = nfa_.newGoalState();
  NfaTransition tr{goal, tok_.multiChar_}; // FIXME: constructor
  nfa_[init].transitions_.emplace_back(std::move(tr));
  return init;
}

} // namespace zezax::red
