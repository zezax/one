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


string DfaObj::fixedPrefix(DfaId &nextId) const {
  string rv;
  nextId   = gDfaInitialId;
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
    nextId = it->second;
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

/* makeEquivalenceMap() - compress alphabet to semantic-preserving minimum

 Insight: If any two characters have different transitions from any state,
          they must be in different equivalence classes.
 Method:  Start with all characters in a single partition.  Iterate all states.
          For each state, iterate all partitions.  Within a partition, any
          character with a non-conforming transition goes to a new partition,
          based on destination state.  When done, each partition is an
          equivalence class.
 Speedup: All characters that don't appear in any transitions form their own
          partition.
 Runtime: Roughly O(nStates * alphabetSize)
          Previous was O(nStates * alphabetSize ^ 2)
          For small alphabets, old approach might be a bit faster, but this
          approach is asymptotically better behaved.
*/
vector<CharIdx> makeEquivalenceMap(const vector<DfaState> &states,
                                   CharIdx                &maxChar, // in-out
                                   MultiChar              &usedChars) {
  CharIdx mx = std::max(gAlphabetSize - 1, maxChar);
  CharIdx mx1 = mx + 1;
  vector<CharIdx> charToPart;
  charToPart.resize(mx1); // zero fill
  if (usedChars.empty()) {
    maxChar = 0;
    return charToPart;
  }

  MultiChar unusedChars = usedChars;
  if (unusedChars.bitSize() < gAlphabetSize)
    unusedChars.resize(gAlphabetSize);
  unusedChars.flipAll(); // now it represents un-used
  for (CharIdx ii = mx + 1; ii < unusedChars.bitSize(); ++ii)
    unusedChars.clear(ii); // clear excess bits in word

  vector<MultiChar> partitions;
  partitions.reserve(mx1); // avoid invalidation
  partitions.emplace_back(std::move(usedChars));
  CharIdx partSize = 1;

  vector<DfaId> flatTrans; // fast-access expansion of unordered_map
  flatTrans.resize(mx1);

  DfaId numStates = static_cast<DfaId>(states.size());
  for (DfaId id = gDfaInitialId; id < numStates; ++id) { // skip error state
    const DfaState &ds = states[id];
    zeroVec(flatTrans);
    for (auto [ch, st] : ds.trans_.getMap())
      flatTrans[ch] = st;

    CharIdx numParts = partSize; // don't iterate the ones we will add
    for (CharIdx curPart = 0; curPart < numParts; ++curPart) {
      MultiChar &curMc = partitions[curPart];
      MultiCharIter mcIt = curMc.begin();
      MultiCharIter mcEnd = curMc.end();
      CharIdx leadChar = *mcIt;
      DfaId leadTrans = flatTrans[leadChar];

      std::map<DfaId, CharIdx> transToPart; // keep track of subdivisions
      for (++mcIt; mcIt != mcEnd; ++mcIt) {
        CharIdx probeChar = *mcIt;
        DfaId probeTrans = flatTrans[probeChar];
        if (probeTrans != leadTrans) { // different fates; must subdivide
          CharIdx destPart;
          auto transIt = transToPart.find(probeTrans);
          if (transIt != transToPart.end()) // already has a destination
            destPart = transIt->second;
          else {
            destPart = partSize++;
            partitions.emplace_back(); // default
            transToPart[probeTrans] = destPart;
          }
          partitions[destPart].set(probeChar);
          curMc.clear(probeChar); // should be OK while iterating
          charToPart[probeChar] = destPart;
        }
      }
    }
  }

  // add partition of unused characters last, to have the largest index
  if (!unusedChars.empty()) {
    for (CharIdx ch : unusedChars)
      charToPart[ch] = partSize;
    partitions.emplace_back(std::move(unusedChars));
    ++partSize;
  }

  maxChar = partSize - 1;
  return charToPart;
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
