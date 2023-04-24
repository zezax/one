// deterministic finite automaton object header

#pragma once

#include <deque>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Consts.h"

namespace zezax::red {

typedef DefaultMap<CharIdx, DfaId>       CharToStateMap;
typedef std::unordered_map<DfaId, DfaId> StateToStateMap;


struct DfaState {
  Result         result_;
  bool           deadEnd_;
  CharToStateMap trans_;
};


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
      for (auto [_, next] : state_->trans_.getMap())
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
      for (auto [_, next] : state_->trans_.getMap())
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


class DfaObj {
public:
  DfaObj() = default;
  ~DfaObj() = default;
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

  Result matchFull(std::string_view s); // for unit tests

  DfaIter iter() { return DfaIter(states_); } // relevant states only
  DfaConstIter citer() const { return DfaConstIter(states_); }

private:
  std::vector<DfaState> states_;
  std::vector<CharIdx>  equivMap_;
};


// useful functions
void flagDeadEnds(std::vector<DfaState> &states, CharIdx maxChar);

std::vector<CharIdx> makeEquivalenceMap(const std::vector<DfaState> &states,
                                        CharIdx                     &maxChar,
                                        MultiChar                   &usedChars);

void remapStates(std::vector<DfaState>      &states,
                 const std::vector<CharIdx> &map);

} // namespace zezax::red
