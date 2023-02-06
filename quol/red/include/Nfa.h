// non-deterministic finite automaton header

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Consts.h"

namespace zezax::red {

struct NfaTransition {
  NfaId     next_;
  MultiChar multiChar_;
};


struct NfaState {
  Result result_;
  std::vector<NfaTransition> transitions_;
};


struct NfaStateTransition {
  NfaId         state_;
  NfaTransition transition_;
};


typedef std::unordered_set<MultiChar> MultiCharSet;


class NfaObj {
public:
  NfaObj() { reset(); }
  ~NfaObj() { freeAll(); }
  NfaObj(const NfaObj &rhs) = delete;
  NfaObj(NfaObj &&rhs);
  NfaObj &operator=(const NfaObj &rhs) = delete;
  NfaObj &operator=(NfaObj &&rhs);

  const NfaState &operator[](NfaId id) const { return states_[id]; }
  NfaState &operator[](NfaId id) { return states_[id]; }

  size_t numStates() const { return states_.size(); }
  size_t activeStates() const;

  void freeAll();

  void setNfaInitial(NfaId id) { initId_ = id; }
  NfaId getNfaInitial() const { return initId_; }

  void setGoal(Result g) { goal_ = g; }
  Result getGoal() const { return goal_; }

  NfaId newState(Result result);
  NfaId newGoalState() { return newState(goal_); }
  NfaId deepCopyState(NfaId id);

  bool accepts(NfaId id) const;
  bool hasAccept(const NfaIdSet &nis) const;

  NfaIdSet allStates(NfaId id) const;
  MultiCharSet allMultiChars(NfaId id) const;
  std::vector<NfaId> allAcceptingStates(NfaId id) const;
  std::vector<NfaStateTransition> allAcceptingTransitions(NfaId id) const;
  void allAcceptingStatesTransitions( // combines above two
      NfaId                            id,
      std::vector<NfaId>              &accStates,
      std::vector<NfaStateTransition> &accTrans) const;

  NfaId stateUnion(NfaId xx, NfaId yy);
  NfaId stateConcat(NfaId xx, NfaId yy);
  NfaId stateOptional(NfaId id);
  NfaId stateKleenStar(NfaId id);
  NfaId stateOneOrMore(NfaId id);
  NfaId stateClosure(NfaId id, int min, int max);
  NfaId stateIgnoreCase(NfaId id);
  NfaId stateWildcard(); // dot-star
  NfaId stateEndMark(CharIdx r);

  void selfUnion(NfaId id);

  void dropUselessTransitions();

private:
  void reset();
  NfaId copyRecurse(std::unordered_map<NfaId, NfaId> &map, NfaId id);

  std::vector<NfaState> states_;
  NfaId                 initId_;
  Result                goal_; // for current regex being added
};

} // namespace zezax::red
