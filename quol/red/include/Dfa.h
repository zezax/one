/* Dfa.h - deterministic finite automaton object header

   DfaObj is a working representation of a DFA.  It's a container of
   states.  It also contains useful functions to operate on and
   transform the DFA.  It includes the map of character equivalence
   classes.

   Two iterators, DfaIter and DfaConstIter provide breadth-first
   traversal of the reachable states.  They also maintain a "seen"
   set of DFA state IDs.

   A matchFull() method is provided in order to verify behavior of
   the DFA at this state of processing.

   In general DfaObj isn't directly useful, but it is used by
   Powerset, DfaMinimizer, and Serializer.  Small tests can be
   composed like this:

   DfaObj dfa;
   DfaId s0 = dfa.newState();
   DfaId s1 = dfa.newState();
   DfaId s2 = dfa.newState();
   dfa[s2].result_ = 1;
   dfa[s1].transitions_.set('a', s2);
   for (DfaConstIter it = dfa.citer(); it; ++it)
     std::cout << it.id() << std::endl;

   DfaObj can throw RedExcept if Budget is exceeded or internal error.
 */

#pragma once

#include <deque>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Consts.h"
#include "Budget.h"

namespace zezax::red {

// CharToStateMap represents the outbound transitions from a state.  It
// indicates the next state based on the input character.
typedef DefaultMap<CharIdx, DfaId>       CharToStateMap;


// DfaState represents a state in the DFA.  Each state has a result;
// positive numbers indicate accepting states.  The dead-end flag is set
// when all possible inputs lead to the same result.  Most important are
// the transitions, a map of input character to next state ID.
struct DfaState {
  Result         result_;
  bool           deadEnd_;
  CharToStateMap transitions_;
};


// StateToStateMap is used for context when transcribing DFAs
typedef std::unordered_map<DfaId, DfaId> StateToStateMap;


class DfaObj; // the main attraction is farther down...


class DfaIter { // unconventional iterator over reachable states (plus zero)
public:
  explicit operator bool() const { return (state_ != nullptr); }

  DfaId id() const { return id_; }
  DfaState &state() const { return *state_; }

  DfaIter &operator++() {
    if (todo_.empty()) {
      id_ = 0;
      state_ = nullptr;
    }
    else {
      id_ = todo_.front(); // breadth-first traversal
      todo_.pop_front();
      state_ = stateBase_ + id_;
      for (auto [_, next] : state_->transitions_.getMap())
        if (!seen_.testAndSet(next))
          todo_.push_back(next);
    }
    return *this;
  }

  DfaIdSet &seen() { return seen_; } // useful by-product

private:
  DfaIter(std::vector<DfaState> &states)
    : id_(0), state_(nullptr), stateBase_(states.data()) {
    DfaId num = std::min(static_cast<DfaId>(states.size()), gDfaInitialId + 1);
    for (DfaId id = 0; id < num; ++id) {
      seen_.insert(id);
      todo_.push_back(id);
    }
    ++*this;
  }

  DfaId              id_;
  DfaState          *state_;
  DfaState          *stateBase_;
  DfaIdSet           seen_;
  std::deque<DfaId>  todo_;

  friend DfaObj;
};


class DfaConstIter { // const version of above
public:
  explicit operator bool() const { return (state_ != nullptr); }

  DfaId id() const { return id_; }
  const DfaState &state() const { return *state_; }

  DfaConstIter &operator++() {
    if (todo_.empty()) {
      id_ = 0;
      state_ = nullptr;
    }
    else {
      id_ = todo_.front(); // breadth-first traversal
      todo_.pop_front();
      state_ = stateBase_ + id_;
      for (auto [_, next] : state_->transitions_.getMap())
        if (!seen_.testAndSet(next))
          todo_.push_back(next);
    }
    return *this;
  }

  DfaIdSet &seen() { return seen_; } // useful by-product

private:
  DfaConstIter(const std::vector<DfaState> &states)
    : id_(0), state_(nullptr), stateBase_(states.data()) {
    DfaId num = std::min(static_cast<DfaId>(states.size()), gDfaInitialId + 1);
    for (DfaId id = 0; id < num; ++id) {
      seen_.insert(id);
      todo_.push_back(id);
    }
    ++*this;
  }

  DfaId              id_;
  const DfaState    *state_;
  const DfaState    *stateBase_;
  DfaIdSet           seen_;
  std::deque<DfaId>  todo_;

  friend DfaObj;
};


// DfaObj holds an array of DFA states.  Each state's ID is its index into
// this array.  ID zero is the error state.  ID one is the initial state.
// DfaObj also holds the map to equivalence classes.  Many operations on the
// DFA are provided for the powerset converter and the minimizer.

class DfaObj {
public:
  explicit DfaObj(Budget *budget = nullptr);
  ~DfaObj();
  DfaObj(const DfaObj &rhs) = delete;
  DfaObj(DfaObj &&rhs) = default;
  DfaObj &operator=(const DfaObj &rhs) = delete;
  DfaObj &operator=(DfaObj &&rhs) = default;

  const DfaState &operator[](DfaId id) const { return states_[id]; }
  DfaState &operator[](DfaId id) { return states_[id]; }

  void clear();
  void reserve(size_t n) { states_.reserve(n); }
  void swap(DfaObj &other);

  DfaId newState();

  DfaIdSet allStateIds() const;
  CharIdx findMaxChar() const;
  CharIdx findUsedChars(MultiChar &used) const; // returns max
  Result findMaxResult() const;

  void chopEndMarks();

  CharIdx installEquivalenceMap(); // returns maxChar
  void copyEquivMap(const DfaObj &src) { equivMap_ = src.equivMap_; }
  const std::vector<CharIdx> &getEquivMap() const { return equivMap_; }

  size_t numStates() const { return states_.size(); }
  const std::vector<DfaState> &getStates() const { return states_; }
  std::vector<DfaState> &getMutStates() { return states_; }

  std::string fixedPrefix(DfaId &nextId) const;

  Budget *getBudget() const { return budget_; }

  Result matchFull(std::string_view s); // for unit tests

  DfaIter iter() { return DfaIter(states_); } // relevant states only
  DfaConstIter citer() const { return DfaConstIter(states_); }

private:
  std::vector<DfaState>  states_;
  std::vector<CharIdx>   equivMap_;
  Budget                *budget_;
};


// useful functions
void flagDeadEnds(std::vector<DfaState> &states, CharIdx maxChar);

std::vector<CharIdx> makeEquivalenceMap(const std::vector<DfaState> &states,
                                        CharIdx                     &maxChar,
                                        MultiChar                   &usedChars);

void remapStates(std::vector<DfaState>      &states,
                 const std::vector<CharIdx> &map);

} // namespace zezax::red
