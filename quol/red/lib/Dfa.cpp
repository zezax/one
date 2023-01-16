// deterministic finite automaton implementation

#include "Dfa.h"

#include <limits>
#include <utility>

#include "Except.h"
#include "Util.h"

namespace zezax::red {

using std::numeric_limits;
using std::string_view;
using std::unordered_map;
using std::vector;

namespace {

void allStatesRecurse(StateIdSet             &seen,
                      const vector<DfaState> &states,
                      StateId                 sid) {
  for (auto [_, id] : states[sid].trans_.getMap())
    if (!seen.testAndSet(id))
      allStatesRecurse(seen, states, id);
}


CharIdx maxCharRecurse(StateIdSet             &seen,
                       const vector<DfaState> &states,
                       StateId                 sid) {
  CharIdx maxChar = 0;
  for (auto [ch, id] : states[sid].trans_.getMap()) {
    if (ch > maxChar)
      maxChar = ch;
    if (!seen.testAndSet(id)) {
      CharIdx sub = maxCharRecurse(seen, states, id);
      if (sub > maxChar)
        maxChar = sub;
    }
  }
  return maxChar;
}


void oneDeadEnd(DfaState &ds, StateId id, CharIdx maxChar) {
  // (likely) try sparse first...
  for (const auto [ch, tid] : ds.trans_.getMap())
    if ((ch < gAlphabetSize) && (tid != id)) {
      ds.deadEnd_ = false;
      return;
    }
  // (rare) then try exhaustive...
  for (CharIdx ch = 0; ch <= maxChar; ++ch)
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

DfaObj transcribeDfa(const DfaObj &src) {
  DfaObj rv;
  StateIdSet states = src.allStateIds();
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

  // copy equivalence map
  rv.copyEquivMap(src);

  return rv;
}


void flagDeadEnds(vector<DfaState> &states, CharIdx maxChar) {
  StateId id = 0;
  for (DfaState &ds : states) {
    oneDeadEnd(ds, id, maxChar);
    ++id;
  }
}


vector<CharIdx> makeEquivalenceMap(const vector<DfaState> &states,
                                   CharIdx                 maxChar) {
  CharIdx limit = maxChar + 1;
  if (limit < gAlphabetSize)
    limit = gAlphabetSize;
  vector<CharIdx> map;
  map.reserve(limit);
  CharIdx cur = 0;

  for (CharIdx ii = 0; ii < limit; ++ii) {
    CharIdx jj;
    for (jj = 0; jj < ii; ++jj)
      if (shareFate(states, ii, jj)) {
        map.push_back(map[jj]);
        break;
      }
    if (jj >= ii)
      map.push_back(cur++);
  }

  return map;
}


void remapStates(vector<DfaState> &states, const vector<CharIdx> &map) {
  if (map.empty())
    throw RedExcept("empty equivalence map");
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
  equivMap_.clear();
}


void DfaObj::swap(DfaObj &other) {
  states_.swap(other.states_);
  equivMap_.swap(other.equivMap_);
}


StateId DfaObj::newState() {
  size_t len = states_.size();
  if (len > numeric_limits<StateId>::max())
    throw RedExcept("DFA state ID overflow");
  states_.resize(len + 1); // default init
  return static_cast<StateId>(len);
}


StateIdSet DfaObj::allStateIds() const {
  StateIdSet seen;
  seen.set(gDfaErrorId);
  seen.set(gDfaInitialId);
  allStatesRecurse(seen, states_, gDfaInitialId);
  return seen;
}


CharIdx DfaObj::findMaxChar() const {
  StateIdSet seen;
  seen.set(gDfaInitialId);
  return maxCharRecurse(seen, states_, gDfaInitialId);
}


void DfaObj::chopEndMarks() {
  for (DfaState &ds : states_) {
    bool first = true;
    CharToStateMap::Map &tmap = ds.trans_.getMap();
    for (auto it = tmap.begin(); it != tmap.end(); )
      if ((it->first >= gAlphabetSize) && (it->second != gDfaErrorId)) {
        if (first) {
          ds.result_ = it->first - gAlphabetSize;
          first = false;
        }
        it = tmap.erase(it);
      }
      else
        ++it;
  }
}


void DfaObj::installEquivalenceMap() {
  vector<CharIdx> map = makeEquivalenceMap(states_, findMaxChar());
  remapStates(states_, map);
  equivMap_.swap(map);
}


Result DfaObj::matchWhole(string_view s) {
  DfaState *cur = &states_[gDfaInitialId];
  for (char c : s) {
    CharIdx ch = static_cast<CharIdx>(c);
    if (ch < equivMap_.size())
      ch = equivMap_[ch];
    cur = &states_[cur->trans_[ch]];
    if (cur->deadEnd_)
      break;
  }
  return cur->result_;
}

} // namespace zezax::red
