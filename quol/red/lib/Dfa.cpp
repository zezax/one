// deterministic finite automaton object implementation

#include "Dfa.h"

#include <limits>
#include <utility>

#include "Except.h"
#include "Util.h"

namespace zezax::red {

using std::numeric_limits;
using std::string;
using std::string_view;
using std::unordered_map;
using std::vector;

namespace {

bool determineDeadEnd(const DfaState &ds, DfaId id, CharIdx maxChar) {
  // (likely) try sparse first...
  const std::unordered_map<CharIdx, DfaId> &sparse = ds.trans_.getMap();
  for (const auto [ch, tid] : sparse)
    if ((ch < gAlphabetSize) && (tid != id))
      return false;
  // if not in sparse, could be default value
  if ((sparse.size() < maxChar) && (id != ds.trans_.getDefault()))
    return false;
  return true;
}


bool shareFate(const vector<DfaState> &states, CharIdx aa, CharIdx bb) {
  for (const DfaState &ds : states)
    if (ds.trans_[aa] != ds.trans_[bb])
      return false;
  return true;
}

} // anonymous

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
    throw RedExceptLimit("dfa state id overflow");
  states_.resize(len + 1); // default init
  return static_cast<DfaId>(len);
}


DfaIdSet DfaObj::allStateIds() const {
  DfaConstIter it = citer();
  for (; it; ++it)
    ;
  return it.seen();
}


CharIdx DfaObj::findMaxChar() const {
  CharIdx high = 0;
  for (DfaConstIter it = citer(); it; ++it)
    for (auto [ch, _] : it.state().trans_.getMap())
      if (ch > high)
        high = ch;
  return high;
}


CharIdx DfaObj::findUsedChars(MultiChar &used) const {
  CharIdx high = 0;
  used.clearAll();
  for (DfaConstIter it = citer(); it; ++it)
    for (auto [ch, _] : it.state().trans_.getMap()) {
      used.set(ch);
      if (ch > high)
        high = ch;
    }
  return high;
}


Result DfaObj::findMaxResult() const {
  Result high = 0;
  for (DfaConstIter it = citer(); it; ++it)
    if (it.state().result_ > high)
      high = it.state().result_;
  return high;
}


void DfaObj::chopEndMarks() {
  for (DfaState &ds : states_) {
    bool found = false;
    CharToStateMap::Map &tmap = ds.trans_.getMap();
    for (auto it = tmap.begin(); it != tmap.end(); )
      if ((it->first >= gAlphabetSize) && (it->second != gDfaErrorId)) {
        if (!found) {
          found = true;
          ds.result_ = it->first - gAlphabetSize;
        }
        it = tmap.erase(it);
      }
      else
        ++it;
  }
}


CharIdx DfaObj::installEquivalenceMap() {
  MultiChar usedChars;
  CharIdx maxChar = findUsedChars(usedChars);
  vector<CharIdx> map = makeEquivalenceMap(states_, maxChar, usedChars);
  remapStates(states_, map);
  equivMap_.swap(map);
  return maxChar;
}


string DfaObj::fixedPrefix() const {
  string rv;
  DfaId id = gDfaInitialId;
  for (int ii = 0; ii < 256; ++ii) { // so length fits in one byte
    const DfaState &ds = states_[id];
    if (ds.result_ > 0) // if we're already accepting, it's not required
      break;
    const std::unordered_map<CharIdx, DfaId> &sparse = ds.trans_.getMap();
    if (sparse.size() != 1) // only looking for unique non-error transitions
      break;
    auto it = sparse.cbegin();
    CharIdx ch = it->first;
    if (ch >= gAlphabetSize)
      break;
    rv.push_back(static_cast<char>(ch));
    if (it->second == id) // avoid loop
      break;
    id = it->second;
  }
  return rv;
}


Result DfaObj::matchFull(string_view sv) {
  const DfaState *ds = &states_[gDfaInitialId];
  for (char c : sv) {
    CharIdx ch = static_cast<unsigned char>(c);
    if (ch < equivMap_.size())
      ch = equivMap_[ch];
    ds = &states_[ds->trans_[ch]];
    if (ds->deadEnd_)
      break;
  }
  return ds->result_;
}

///////////////////////////////////////////////////////////////////////////////

void flagDeadEnds(vector<DfaState> &states, CharIdx maxChar) {
  DfaId id = 0;
  for (DfaState &ds : states)
    ds.deadEnd_ = determineDeadEnd(ds, id++, maxChar);
}


// this can be a performance-sensitive function
vector<CharIdx> makeEquivalenceMap(const vector<DfaState> &states,
                                   CharIdx                &maxChar, // in-out
                                   MultiChar              &charMask) {
  CharIdx limit = maxChar + 1;
  CharIdx size = std::max(limit, gAlphabetSize);
  vector<CharIdx> map;
  map.reserve(size);
  CharIdx cur = 0;

  CharIdx unused;
  if (charMask.bitSize() == 0)
    unused = 0;
  else {
    charMask.flipAll();         // now represents un-used chars
    auto it = charMask.begin(); // look for first unused character
    if (it == charMask.end())
      unused = numeric_limits<CharIdx>::max();
    else
      unused = *it;
  }

  CharIdx ii = 0;
  for (; ii < limit; ++ii) {
    CharIdx jj;
    if (!charMask.get(ii)) { // one is used -> we really need to check fates
      for (jj = 0; jj < ii; ++jj)
        if (shareFate(states, ii, jj)) {
          map.push_back(map[jj]);
          break;
        }
    }
    else {                   // one is not used -> we can cheat
      jj = unused;           // should have same fate as any other unused
      if (jj < ii)
        map.push_back(map[jj]);
    }

    if (jj >= ii)
      map.push_back(cur++);
  }

  if (ii < size) {            // every char past limit is unused
    CharIdx val;
    if (unused != numeric_limits<CharIdx>::max())
      val = map[unused];
    else
      val = cur++;
    for (; ii < size; ++ii)
      map.push_back(val);
  }

  maxChar = (cur == 0) ? 0 : cur - 1;
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

} // namespace zezax::red
