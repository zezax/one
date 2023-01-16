// deterministic finite automaton header

#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "Defs.h"
#include "Container.h"
#include "BitSet.h"

namespace zezax::red {

constexpr StateId gDfaErrorId   = 0;
constexpr StateId gDfaInitialId = 1;

typedef DefaultMap<CharIdx, StateId>         CharToStateMap;
typedef std::unordered_map<StateId, StateId> StateToStateMap;


struct DfaState {
  Result         result_;
  bool           deadEnd_;
  CharToStateMap trans_;
};


class DfaObj {
public:
  DfaObj() = default;
  ~DfaObj() = default;
  DfaObj(const DfaObj &rhs) = delete;
  DfaObj(DfaObj &&rhs) = default;
  DfaObj &operator=(const DfaObj &rhs) = delete;
  DfaObj &operator=(DfaObj &&rhs) = default;

  const DfaState &operator[](StateId id) const { return states_[id]; }
  DfaState &operator[](StateId id) { return states_[id]; }

  void clear();
  void reserve(size_t n) { states_.reserve(n); }
  void swap(DfaObj &other);

  StateId newState();

  StateIdSet allStateIds() const;
  CharIdx findMaxChar() const;
  Result findMaxResult() const;

  void chopEndMarks();

  void installEquivalenceMap();
  void copyEquivMap(const DfaObj &src) { equivMap_ = src.equivMap_; }
  const std::vector<CharIdx> &getEquivMap() const { return equivMap_; }

  size_t numStates() const { return states_.size(); }
  const std::vector<DfaState> &getStates() const { return states_; }
  std::vector<DfaState> &getMutStates() { return states_; }

  Result matchWhole(std::string_view s); // for unit tests

private:
  std::vector<DfaState> states_;
  std::vector<CharIdx>  equivMap_;
};


// useful functions
CharIdx findMaxChar(const std::vector<DfaState> &states);
DfaObj transcribeDfa(const DfaObj &src);
void flagDeadEnds(std::vector<DfaState> &states, CharIdx maxChar);

std::vector<CharIdx> makeEquivalenceMap(const std::vector<DfaState> &states,
                                        CharIdx                      maxChar);

void remapStates(std::vector<DfaState>      &states,
                 const std::vector<CharIdx> &map);

} // namespace zezax::red
