// deterministic finite automaton object header

#pragma once

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

  Result matchFull(std::string_view s); // for unit tests

private:
  std::vector<DfaState> states_;
  std::vector<CharIdx>  equivMap_;
};


// useful functions
void flagDeadEnds(std::vector<DfaState> &states, CharIdx maxChar);

std::vector<CharIdx> makeEquivalenceMap(const std::vector<DfaState> &states,
                                        CharIdx                     &maxChar,
                                        MultiChar                   &charMask);

void remapStates(std::vector<DfaState>      &states,
                 const std::vector<CharIdx> &map);

} // namespace zezax::red
