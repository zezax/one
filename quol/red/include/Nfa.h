// non-deterministic finite automaton header

#pragma once

#include <unordered_set>
#include <vector>

#include "Defs.h"
#include "BitSet.h"

namespace zezax::red {

struct NfaState;

struct NfaTransition {
  NfaState *next_;
  MultiChar multiChar_;
};


struct NfaState {
  StateId id_;
  Result result_;
  NfaState *allocNext_;
  std::vector<NfaTransition> transitions_;
};


class NfaObj {
public:
  NfaObj() { reset(); }
  ~NfaObj() { freeAll(); }
  NfaObj(const NfaObj &rhs) = delete;
  NfaObj(NfaObj &&rhs);
  NfaObj &operator=(const NfaObj &rhs) = delete;
  NfaObj &operator=(NfaObj &&rhs);

  void freeAll();

  void setNfaInitial(NfaState *ns) { nfa_ = ns; }
  NfaState *getNfaInitial() const { return nfa_; }

  void setGoal(Result g) { goal_ = g; }
  Result getGoal() const { return goal_; }

  NfaState *newState(Result result);
  NfaState *newGoalState() { return newState(goal_); }
  NfaState *deepCopyState(NfaState *ns);

  NfaState *stateUnion(NfaState *xx, NfaState *yy);
  NfaState *stateConcat(NfaState *xx, NfaState *yy);
  NfaState *stateOptional(NfaState *ns);
  NfaState *stateKleenStar(NfaState *ns);
  NfaState *stateOneOrMore(NfaState *ns);
  NfaState *stateCount(NfaState *ns, int min, int max);
  NfaState *stateIgnoreCase(NfaState *ns);
  NfaState *stateWildcard(); // .*
  NfaState *stateEndMark(CharIdx r);

  void selfUnion(NfaState *ns);

private:
  void reset();

  NfaState *nfa_;
  NfaState *alloc_; // linked list for freeing
  StateId   curId_;
  Result    goal_; // for current regex being added
};


struct NfaStateTransition {
  NfaState *state_;
  NfaTransition transition_;
};


typedef std::unordered_set<NfaState *> NfaStateSet;
typedef std::unordered_set<MultiChar> MultiCharSet;

// useful functions...
bool accepts(const NfaState *ns);
bool hasAccept(const NfaStateSet &ss);
size_t hashState(const NfaState *ns);
NfaStateSet allStates(NfaState *ns);
std::vector<NfaState *> allAcceptingStates(NfaState *ns);
std::vector<NfaStateTransition> allAcceptingTransitions(NfaState *ns);
MultiCharSet allMultiChars(NfaState *ns);

} // namespace zezax::red

// for use in std::unordered_map
template<> struct std::hash<zezax::red::NfaStateSet> {
  size_t operator()(const zezax::red::NfaStateSet &ss) const {
    size_t rv = 0;
    for (zezax::red::NfaState *ns : ss)
      rv ^= zezax::red::hashState(ns); // order-independent
    return rv;
  }
};
