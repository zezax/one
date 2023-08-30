/* Nfa.h - non-deterministic finite automaton object header

   NfaObj is a representation of an NFA.  It's a container of states.
   It's also the the intermediate representation of a regular
   expression, produced by Parser and consumed by Powerset, which
   converts it to a DFA.

   Provided are convenience functions for building the NFA, as well
   as functions to transform and inspect the NFA.

   Two iterators, NfaIter and NfaConstIter provide breadth-first
   traversal of the reachable states.  They also maintain a "seen"
   set of NFA state IDs.

   In general NfaObj isn't directly useful, but it is used by Parser
   and Powerset.  Small tests can be composed like this:

   NfaObj nfa;
   NfaId s1 = nfa.newState(0);
   NfaId s2 = nfa.newState(1);
   NfaTransition x;
   x.next_ = s2;
   x.multiChar_.insert('a');
   nfa[s1].transitions_.emplace_back(std::move(x));
   nfa.setInitial(s1);
   nfa.setGoal(s2);
   for (NfaConstIter it = nfa.citer(); it; ++it)
     std::cout << it.id() << std::endl;

   NfaObj can throw RedExcept if Budget or other limits apply.
 */

#pragma once

#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Consts.h"
#include "Budget.h"

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


inline bool operator==(const NfaTransition &aa, const NfaTransition &bb) {
  return ((aa.next_ == bb.next_) && (aa.multiChar_ == bb.multiChar_));
}


typedef std::unordered_set<MultiChar> MultiCharSet;


class NfaObj; // the important stuff is farther down...


class NfaIter { // not your normal iterator; connected states only
public:
  explicit operator bool() const { return (state_ != nullptr); }

  NfaId id() const { return id_; }
  NfaState &state() const { return *state_; }

  NfaIter &operator++() {
    if (todo_.empty()) {
      id_ = 0;
      state_ = nullptr;
    }
    else {
      id_ = todo_.front(); // breadth-first iteration
      todo_.pop_front();
      state_ = stateBase_ + id_;
      for (const NfaTransition &tr : state_->transitions_)
        if (!seen_.testAndSet(tr.next_))
          todo_.push_back(tr.next_);
    }
    return *this;
  }

  NfaIdSet &seen() { return seen_; } // useful by-product

private:
  NfaIter(NfaId id, std::vector<NfaState> &states)
    : id_(0), state_(nullptr), stateBase_(states.data()) {
    if (!states.empty()) {
      seen_.insert(id);
      todo_.push_back(id);
    }
    ++*this;
  }

  NfaId              id_;
  NfaState          *state_;
  NfaState          *stateBase_;
  NfaIdSet           seen_;
  std::deque<NfaId>  todo_;

  friend NfaObj;
};


class NfaConstIter { // const variant
public:
  explicit operator bool() const { return (state_ != nullptr); }

  NfaId id() const { return id_; }
  const NfaState &state() const { return *state_; }

  NfaConstIter &operator++() {
    if (todo_.empty()) {
      id_ = 0;
      state_ = nullptr;
    }
    else {
      id_ = todo_.front(); // breadth-first iteration
      todo_.pop_front();
      state_ = stateBase_ + id_;
      for (const NfaTransition &tr : state_->transitions_)
        if (!seen_.testAndSet(tr.next_))
          todo_.push_back(tr.next_);
    }
    return *this;
  }

  NfaIdSet &seen() { return seen_; } // useful by-product

private:
  NfaConstIter(NfaId id, const std::vector<NfaState> &states)
    : id_(0), state_(nullptr), stateBase_(states.data()) {
    if (!states.empty()) {
      seen_.insert(id);
      todo_.push_back(id);
    }
    ++*this;
  }

  NfaId              id_;
  const NfaState    *state_;
  const NfaState    *stateBase_;
  NfaIdSet           seen_;
  std::deque<NfaId>  todo_;

  friend NfaObj;
};


class NfaObj {
public:
  explicit NfaObj(Budget *budget = nullptr);
  ~NfaObj();
  NfaObj(const NfaObj &rhs) = delete;
  NfaObj(NfaObj &&rhs);
  NfaObj &operator=(const NfaObj &rhs) = delete;
  NfaObj &operator=(NfaObj &&rhs);

  const NfaState &operator[](NfaId id) const { return states_[id]; }
  NfaState &operator[](NfaId id) { return states_[id]; }

  size_t numStates() const { return states_.size(); }
  size_t activeStates() const;

  void freeAll();

  void setInitial(NfaId id) { initId_ = id; }
  NfaId getInitial() const { return initId_; }

  void setGoal(Result g) { goal_ = g; }
  Result getGoal() const { return goal_; }

  NfaId newState(Result result);
  NfaId newGoalState() { return newState(goal_); }
  NfaId deepCopyState(NfaId id);

  bool accepts(NfaId id) const;
  bool hasAccept(const NfaIdSet &nis) const;

  NfaIdSet allStates(NfaId id) const;
  MultiCharSet allMultiChars(NfaId id) const;
  std::vector<NfaStateTransition> allAcceptingTransitions(NfaId id) const;
  void allAcceptingStatesTransitions( // combines above two
      NfaId                            id,
      std::vector<NfaId>              &accStates,
      std::vector<NfaStateTransition> &accTrans) const;

  NfaId stateUnion(NfaId xx, NfaId yy);
  NfaId stateConcat(NfaId xx, NfaId yy);
  NfaId stateOptional(NfaId id);
  NfaId stateKleenStar(NfaId id);
  NfaId stateClosure(NfaId id, int min, int max);
  NfaId stateIgnoreCase(NfaId id);
  NfaId stateWildcard(); // dot-star
  NfaId stateEndMark(CharIdx r);

  void selfUnion(NfaId id);

  void dropUselessTransitions();

  NfaIter iter(NfaId id) { return NfaIter(id, states_); }
  NfaConstIter citer(NfaId id) const { return NfaConstIter(id, states_); }

private:
  NfaId copyRecurse(std::unordered_map<NfaId, NfaId> &map, NfaId id);

  std::vector<NfaState>  states_;
  Budget                *budget_;
  NfaId                  initId_;
  Result                 goal_; // for current regex being added
};

} // namespace zezax::red
