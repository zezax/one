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

void allStatesRecurse(DfaIdSet               &seen,
                      const vector<DfaState> &states,
                      DfaId                   did) {
  for (auto [_, id] : states[did].trans_.getMap())
    if (!seen.testAndSet(id))
      allStatesRecurse(seen, states, id);
}


CharIdx maxCharRecurse(DfaIdSet               &seen,
                       const vector<DfaState> &states,
                       DfaId                   did) {
  CharIdx maxChar = 0;
  for (auto [ch, id] : states[did].trans_.getMap()) {
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


Result maxResultRecurse(DfaIdSet               &seen,
                        const vector<DfaState> &states,
                        DfaId                   did) {
  const DfaState &ds = states[did];
  Result maxResult = ds.result_;
  for (auto [_, id] : ds.trans_.getMap())
    if (!seen.testAndSet(id)) {
      Result sub = maxResultRecurse(seen, states, id);
      if (sub > maxResult)
        maxResult = sub;
    }

  return maxResult;
}


void oneDeadEnd(DfaState &ds, DfaId id, CharIdx maxChar) {
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
  DfaIdSet states = src.allStateIds();
  rv.reserve(states.population());

  StateToStateMap oldToNew;
  DfaId errId = rv.newState();
  DfaId initId = rv.newState();
  if ((errId != gDfaErrorId) || (initId != gDfaInitialId))
    throw RedExceptMinimize("dfa state ids not as expected");
  oldToNew.emplace(gDfaErrorId, gDfaErrorId);
  oldToNew.emplace(gDfaInitialId, gDfaInitialId);

  // this skips unreachable states
  for (DfaId srcId : states) {
    if (!oldToNew.contains(srcId)) {
      DfaId newId = rv.newState();
      oldToNew[srcId] = newId;
    }
  }

  for (DfaId srcId : states) {
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
  DfaId id = 0;
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
    throw RedExceptCompile("empty equivalence map");
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


DfaId DfaObj::newState() {
  size_t len = states_.size();
  if (len > numeric_limits<DfaId>::max())
    throw RedExceptLimit("DFA state ID overflow");
  states_.resize(len + 1); // default init
  return static_cast<DfaId>(len);
}


DfaIdSet DfaObj::allStateIds() const {
  DfaIdSet seen;
  seen.set(gDfaErrorId);
  seen.set(gDfaInitialId);
  allStatesRecurse(seen, states_, gDfaInitialId);
  return seen;
}


CharIdx DfaObj::findMaxChar() const {
  DfaIdSet seen;
  seen.set(gDfaInitialId);
  return maxCharRecurse(seen, states_, gDfaInitialId);
}


Result DfaObj::findMaxResult() const {
  DfaIdSet seen;
  seen.set(gDfaInitialId);
  return maxResultRecurse(seen, states_, gDfaInitialId);
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
