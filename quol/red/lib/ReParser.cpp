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
  if (result <= 0)
    throw RedExceptApi("result must be positive");

  obj_.setGoal(result);
  flags_ = flags;
  level_ = 0;

  NfaId startWild = gNfaNullId;
  if (flags_ & fLooseStart)
    startWild = obj_.stateWildcard(); // do early for proper numbering

  scanner_.init(regex);
  advance();
  NfaId state = parseExpr();

  if (!state)
    throw RedExceptParse("uncategorized parsing error");
  if (tok_.type_ != tEnd)
    throw RedExceptParse("trailing unparsed input");

  if (flags_ & fIgnoreCase)
    state = obj_.stateIgnoreCase(state);

  if (startWild)
    state = obj_.stateConcat(startWild, state);
  if (flags_ & fLooseEnd)
    state = obj_.stateConcat(state, obj_.stateWildcard());

  state = obj_.stateConcat(state, obj_.stateEndMark(result));
  obj_.selfUnion(state); // all added regexes are acceptable
}


void ReParser::addAuto(string_view regex, Result result, FlagsT flags) {
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
  obj_.dropUselessTransitions();
}


void ReParser::freeAll() {
  obj_.freeAll();
}

///////////////////////////////////////////////////////////////////////////////

NfaId ReParser::parseExpr() {
  NfaId state = parseTerm();
  if (!state)
    return gNfaNullId;

  while (tok_.type_ == tBar) {
    advance();
    NfaId alt = parseTerm();
    if (!alt)
      return gNfaNullId;
    state = obj_.stateUnion(state, alt);
  }
  return state;
}


NfaId ReParser::parseTerm() {
  validated_ = false;
  NfaId state = parseFactor();
  if (!state)
    return gNfaNullId;

  while ((tok_.type_ == tChars) || (tok_.type_ == tLeft)) {
    NfaId more = parseFactor();
    if (!more)
      return gNfaNullId;
    state = obj_.stateConcat(state, more);
  }
  return state;
}


NfaId ReParser::parseFactor() {
  NfaId state = parseAtom();
  if (!state)
    return gNfaNullId;

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


NfaId ReParser::parseAtom() {
  while (tok_.type_ == tFlags) {
    flags_ |= tok_.flags_;
    advance();
  }

  NfaId state = gNfaNullId;

  switch (tok_.type_) {
  case tEnd:
    if (validated_)
      throw RedExceptParse("end reached, expected chars or (", tok_.pos_);
    return obj_.newGoalState(); // empty matches all
  case tChars:
    validated_ = true;
    state = parseCharBits();
    break;
  case tBar:
    if (validated_)
      throw RedExceptParse("expected chars or (, got |", tok_.pos_);
    return obj_.newGoalState(); // empty matches all
  case tLeft:
    advance();
    ++level_;
    state = parseExpr();
    --level_;
    if (tok_.type_ != tRight)
      throw RedExceptParse("close parenthesis not found", tok_.pos_);
    validated_ = true;
    break;
  case tRight:
    if (validated_ || (level_ <= 0))
      throw RedExceptParse("unexpected )", tok_.pos_);
    return obj_.newGoalState(); // empty matches all
  default:
    throw RedExceptParse("expected chars or (", tok_.pos_);
  }

  advance();
  return state;
}


NfaId ReParser::parseCharBits() {
  NfaId init = obj_.newState(0);
  NfaId goal = obj_.newGoalState();
  NfaTransition tr{goal, tok_.multiChar_}; // FIXME: constructor
  obj_[init].transitions_.emplace_back(std::move(tr));
  return init;
}

} // namespace zezax::red
