// regular expression non-deterministic finite automaton parser implementation

// expr = term
// expr = term | term
// term = factor
// term = factor factor
// factor = atom
// factor = atom count
// factor = atom flag
// atom = lparen expr rparen
// atom = charbits

// TODO:
// atom -> unit
// factor -> poly (multi)
// term -> part
// count -> closure

#include "ReParser.h"

#include "Except.h"

namespace zezax::red {

using std::string_view;

ReParser::ReParser()
: flags_(0),
  level_(0),
  validated_(false),
  tok_(tError, gNoPos) {}


void ReParser::add(string_view regex, Result result, FlagsT flags) {
  if (regex.starts_with("\\i")) {
    regex.remove_prefix(2);
    flags |= fIgnoreCase;
  }

  if (regex.starts_with('^')) {
    regex.remove_prefix(1);
    flags |= fAnchorStart;
  }
  else if (regex.starts_with(".*")) {
    regex.remove_prefix(2);
    flags &= ~fAnchorStart;
  }

  if (regex.ends_with('$')) {
    regex.remove_suffix(1);
    flags |= fAnchorEnd;
  }
  else if (regex.ends_with(".*")) {
    regex.remove_suffix(2);
    flags &= ~fAnchorEnd;
  }

  addRaw(regex, result, flags);
}


void ReParser::addRaw(string_view regex, Result result, FlagsT flags) {
  if (result <= 0)
    throw RedExcept("result must be positive");

  obj_.setGoal(result);
  flags_ = flags;
  level_ = 0;

  NfaState *startWild = nullptr;
  if ((flags_ & fAnchorStart) == 0)
    startWild = obj_.stateWildcard(); // do early for proper numbering

  scanner_.init(regex);
  advance();
  NfaState *state = parseExpr();

  if (!state)
    throw RedExcept("uncategorized parsing error");
  if (tok_.type_ != tEnd)
    throw RedExcept("trailing unparsed input");

  if (flags_ & fIgnoreCase)
    state = obj_.stateIgnoreCase(state);

  if (startWild)
    state = obj_.stateConcat(startWild, state);
  if ((flags_ & fAnchorEnd) == 0)
    state = obj_.stateConcat(state, obj_.stateWildcard());

  state = obj_.stateConcat(state, obj_.stateEndMark(result));
  obj_.selfUnion(state); // all added regexes are acceptable
}


NfaState *ReParser::parseExpr() {
  NfaState *state = parseTerm();
  if (!state)
    return nullptr;

  while (tok_.type_ == tBar) {
    advance();
    NfaState *alt = parseTerm();
    if (!alt)
      return nullptr;
    state = obj_.stateUnion(state, alt);
  }
  return state;
}


NfaState *ReParser::parseTerm() {
  validated_ = false;
  NfaState *state = parseFactor();
  if (!state)
    return nullptr;

  while ((tok_.type_ == tChars) || (tok_.type_ == tLeft)) {
    NfaState *more = parseFactor();
    if (!more)
      return nullptr;
    state = obj_.stateConcat(state, more);
  }
  return state;
}


NfaState *ReParser::parseFactor() {
  NfaState *state = parseAtom();
  if (!state)
    return nullptr;

  switch (tok_.type_) {
  case tClosure:
    state = obj_.stateCount(state, tok_.min_, tok_.max_);
    break;
  case tFlags:
    flags_ |= tok_.flags_;
    break;
  default:
    return state;
  }

  advance();
  while (tok_.type_ == tFlags) {
    flags_ |= tok_.flags_;
    advance();
  }

  return state;
}


NfaState *ReParser::parseAtom() {
  while (tok_.type_ == tFlags) {
    flags_ |= tok_.flags_;
    advance();
  }

  NfaState *state = nullptr;

  switch (tok_.type_) {
  case tEnd:
    if (validated_)
      throw RedExcept("end reached, expected chars or (", tok_.pos_);
    return obj_.newGoalState(); // empty matches all
  case tChars:
    validated_ = true;
    state = parseCharBits();
    break;
  case tBar:
    if (validated_)
      throw RedExcept("expected chars or (, got |", tok_.pos_);
    return obj_.newGoalState(); // empty matches all
  case tLeft:
    advance();
    ++level_;
    state = parseExpr();
    --level_;
    if (tok_.type_ != tRight)
      throw RedExcept("close parenthesis not found", tok_.pos_);
    validated_ = true;
    break;
  case tRight:
    if (validated_ || (level_ <= 0))
      throw RedExcept("unexpected )", tok_.pos_);
    return obj_.newGoalState(); // empty matches all
  default:
    throw RedExcept("expected chars or (", tok_.pos_);
  }

  advance();
  return state;
}


NfaState *ReParser::parseCharBits() {
  NfaState *init = obj_.newState(0);
  NfaState *goal = obj_.newGoalState();
  NfaTransition tr{goal, tok_.multiChar_}; // FIXME: constructor
  init->transitions_.emplace_back(std::move(tr));
  return init;
}

} // namespace zezax::red
