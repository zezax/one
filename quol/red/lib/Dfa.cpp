// deterministic finite automaton implementation

#include "Dfa.h"

#include <utility>

#include "Except.h"
#include "Util.h"

namespace zezax::red {

using std::numeric_limits;
using std::string_view;
using std::vector;

namespace {

void allStatesRecurse(StateIdSet &seen,
                      const std::vector<DfaState> &states,
                      StateId sid) {
  const DfaState &ds = states[sid];
  for (auto [_, id] : ds.trans_.getMap())
    if (!seen.testAndSet(id))
      allStatesRecurse(seen, states, id);
}


void oneDeadEnd(DfaState &ds, StateId id) {
  // (likely) try sparse first...
  for (const auto [ch, tid] : ds.trans_.getMap())
    if ((ch < gAlphabetSize) && (tid != id)) {
      ds.deadEnd_ = false;
      return;
    }
  // (rare) then try exhaustive...
  for (CharIdx ch = 0; ch < gAlphabetSize; ++ch)
    if (ds.trans_[ch] != id) {
      ds.deadEnd_ = false;
      return;
    }
  ds.deadEnd_ = true;
}


bool shareFate(const vector<DfaState> &states, CharIdx aa, CharIdx bb) {
  for (const DfaState &ds : states)
    if (ds.trans_[aa] != ds.trans_[bb])
      return false;
  return true;
}

} // anonymous

StateIdSet allStates(const std::vector<DfaState> &states) {
  StateIdSet seen;
  seen.set(gDfaErrorId);
  seen.set(gDfaInitialId);
  allStatesRecurse(seen, states, gDfaInitialId);
  return seen;
}


DfaObj transcribeDfa(const DfaObj &src) {
  DfaObj rv;
  StateIdSet states = allStates(src.getStates());
  rv.reserve(states.population());

  StateToStateMap oldToNew;
  StateId errId = rv.newState();
  StateId initId = rv.newState();
  if ((errId != gDfaErrorId) || (initId != gDfaInitialId))
    throw RedExcept("dfa state ids not as expected");
  oldToNew.emplace(gDfaErrorId, gDfaErrorId);
  oldToNew.emplace(gDfaInitialId, gDfaInitialId);

  // this skips unreachable states
  for (StateId srcId : states) {
    if (!oldToNew.contains(srcId)) {
      StateId newId = rv.newState();
      oldToNew[srcId] = newId;
    }
  }

  for (StateId srcId : states) {
    const DfaState &srcState = src[srcId];
    DfaState &newState = rv[oldToNew[srcId]];
    for (auto srcIt = srcState.trans_.getMap().cbegin();
         srcIt != srcState.trans_.getMap().cend();
         ++srcIt)
      newState.trans_.emplace(srcIt->first, oldToNew[srcIt->second]);
    newState.result_ = srcState.result_;
    newState.deadEnd_ = srcState.deadEnd_;
  }

  return rv;
}


void flagDeadEnds(vector<DfaState> &states) {
  StateId id = 0;
  for (DfaState &ds : states) {
    oneDeadEnd(ds, id);
    ++id;
  }
}


// FIXME: put in minimizer/ executable???
vector<CharIdx> makeEquivalenceMap(const vector<DfaState> &states) {
  vector<CharIdx> map;
  map.reserve(gAlphabetSize);
  CharIdx cur = 0;

  for (CharIdx ii = 0; ii < gAlphabetSize; ++ii) {
    CharIdx jj;
    for (jj = 0; jj < ii; ++jj)
      if (shareFate(states, ii, jj)) {
        safeRef(map, ii) = safeRef(map, jj);
        break;
      }
    if (jj >= ii)
      safeRef(map, ii) = cur++;
  }

  return map;
}


void remapStates(vector<DfaState> &states, const vector<CharIdx> &map) {
  if (map.empty())
    return;
  for (DfaState &ds : states) {
    CharToStateMap work;
    for (auto [ch, id] : ds.trans_.getMap())
      work.set(map[ch], id);
    ds.trans_.swap(work);
  }
}

///////////////////////////////////////////////////////////////////////////////

void DfaObj::clear() {
  states_.clear();
}


void DfaObj::swap(DfaObj &other) {
  states_.swap(other.states_);
}


StateId DfaObj::newState() {
  size_t len = states_.size();
  states_.resize(len + 1); // default init
  return static_cast<StateId>(len);
}


StateIdSet DfaObj::allStateIds() const {
  return allStates(states_);
}


void DfaObj::useEquivalenceMap() {
  //FIXME: store map in obj
  vector<CharIdx> map = makeEquivalenceMap(states_);
  remapStates(states_, map);
}


Result DfaObj::match(string_view s) {
  // FIXME: char groups???
  DfaState *cur = &states_[gDfaInitialId];
  for (char c : s) {
    CharIdx ch = static_cast<CharIdx>(c);
    cur = &states_[cur->trans_[ch]];
    if (cur->deadEnd_)
      break;
  }
  return cur->result_;
}

} // namespace zezax::red
