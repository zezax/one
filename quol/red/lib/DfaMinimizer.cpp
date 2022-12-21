// dfa minimizer implementation after david gries 1973

// Note: This is used for DFAs with end marks.  Untested without.

#include "DfaMinimizer.h"

#include <cassert>

#include "Except.h"
#include "Dfa.h"

#include "Debug.h" // FIXME

namespace zezax::red {

using std::unordered_map;
using std::vector;

namespace {

constexpr BlockId gNormalBlock = 0;
constexpr BlockId gAcceptBlock = 1;

bool accepts(const DfaState &ds) { return (ds.result_ > 0); }


StateId &twinRef(vector<StateId> &vec, size_t idx) {
  while (vec.size() <= idx)
    vec.push_back(-1);
  return vec[idx];
}


BlockId findInBlock(StateId id, const vector<StateIdSet> &blocks) {
  for (BlockId block = 0; static_cast<size_t>(block) < blocks.size(); ++block)
    if (blocks[block].get(id))
      return block;
  throw RedExcept("no block containing state");
}

} // anonymous

CharIdx findMaxChar(const vector<DfaState> &stateVec) {
  // FIXME: do we need to BFS from init state?
  CharIdx maxChar = 0;
  for (const DfaState &ds : stateVec)
    for (auto [ch, _] : ds.trans_.getMap()) // only non-default chars
      if (ch > maxChar)
        maxChar = ch;
  return maxChar;
}


void pullBackEndMarks(vector<DfaState> &states) {
  for (DfaState &ds : states)
    for (auto [ch, id] : ds.trans_.getMap())
      if ((ch >= gAlphabetSize) && (id != gDfaErrorId))
        ds.result_ = ch - gAlphabetSize;
}


void chopEndMarks(vector<DfaState> &states) {
  for (DfaState &ds : states) {
    bool first = true;
    unordered_map<CharIdx, StateId>  &tmap = ds.trans_.getMap();
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


DfaEdgeToIds invert(const StateIdSet       &stateSet,
                    const vector<DfaState> &stateVec,
                    CharIdx                 maxChar) {
  DfaEdgeToIds rv;

  for (StateId sid : stateSet) {
    const DfaState &ds = stateVec[sid];
    // FIXME: does it work if we ignore edges to zero???
    for (CharIdx ch = 0; ch <= maxChar; ++ch) {
      std::pair<DfaEdge, StateIdSet> node;
      node.first.id_ = ds.trans_[ch];
      node.first.char_ = ch;
      auto [it, _] = rv.emplace(std::move(node));
      it->second.set(sid);
    }
  }

  return rv;
}


void partition(const StateIdSet       &stateSet,
               const vector<DfaState> &stateVec,
               StateIdSet             &normal,
               StateIdSet             &accept) {
  for (StateId sid : stateSet) {
    const DfaState &ds = stateVec[sid];
    if (accepts(ds))
      accept.set(sid);
    else
      normal.set(sid);
  }
}


BlockRecSet makeList(CharIdx           maxChar,
                     const StateIdSet &normal,
                     const StateIdSet &accept) {
  BlockRecSet list;
  BlockRec br;
  br.block_ = gNormalBlock;

  if (accept.size() < normal.size())
    br.block_ = gAcceptBlock;
  for (CharIdx ch = 0; ch <= maxChar; ++ch) {
    br.char_ = ch;
    list.insert(br);
  }

  return list;
}


StateIdSet locateSplits(const BlockRec           &blockRec,
                        const vector<StateIdSet> &blocks,
                        const DfaEdgeToIds       &inv) {
  StateIdSet rv;
  DfaEdge key;
  key.char_ = blockRec.char_;
  for (StateId id : blocks[blockRec.block_]) {
    key.id_ = id;
    auto it = inv.find(key);
    if (it != inv.end())
      rv.unionWith(it->second);
  }
  return rv;
}


void performSplits(const BlockRec         &blockRec,
                   const StateIdSet       &splits,
                   vector<BlockId>        &twins,
                   PatchSet               &patches,
                   const vector<DfaState> &stateVec,
                   vector<StateIdSet>     &blocks) {
  twins.clear(); // forget old
  for (StateId splitId : splits) {
    BlockId blockNum = findInBlock(splitId, blocks);
    if (!containedIn(blockNum, blockRec.block_, blockRec.char_,
                     stateVec, blocks))
      handleTwins(splitId, blockNum, blocks, twins, patches);
  }
}


bool containedIn(BlockId                   needleId,
                 BlockId                   haystackId,
                 CharIdx                   ch,
                 const vector<DfaState>   &stateVec,
                 const vector<StateIdSet> &blocks) {
  const StateIdSet &needle = blocks[needleId];
  const StateIdSet &haystack = blocks[haystackId];

  for (StateId id : needle) {
    const DfaState &ds = stateVec[id];
    if (!haystack.get(ds.trans_[ch]))
      return false;
  }

  return true;
}


void handleTwins(StateId             stateId,
                 BlockId             blockId,
                 vector<StateIdSet> &blocks,
                 vector<BlockId>    &twins,
                 PatchSet           &patches) {
  BlockId twin = twinRef(twins, blockId);

  if (twin == -1) {
    twin = static_cast<BlockId>(blocks.size());
    twinRef(twins, blockId) = twin;
    blocks.emplace_back(); // default
  }

  blocks[blockId].clear(stateId);
  blocks[twin].set(stateId);
  patches.emplace(blockId, twin);
}


void patchBlocks(PatchSet                 &patches,
                 BlockRecSet              &list,
                 CharIdx                   maxChar,
                 const vector<StateIdSet> &blocks) {
  for (const Patch &patch : patches)
    patchPair(patch.first, patch.second, list, maxChar, blocks);
  patches.clear();
}


void patchPair(BlockId ii,
               BlockId jj,
               BlockRecSet &list,
               CharIdx maxChar,
               const vector<StateIdSet> &blocks) {
  BlockRec bii;
  bii.block_ = ii;
  BlockRec bjj;
  bjj.block_ = jj;
  BlockRec bmin;
  bmin.block_ = (blocks[ii].population() < blocks[jj].population()) ? ii : jj;

  for (CharIdx ch = 0; ch <= maxChar; ++ch) {
    bii.char_ = ch;
    if (list.contains(bii)) {
      bjj.char_ = ch;
      list.insert(bjj);
    }
    else {
      bmin.char_ = ch;
      list.insert(bmin);
    }
  }
}


void finalizeDfa(const DfaObj &srcDfa,
                 DfaObj &outDfa,
                 const vector<StateIdSet> &blocks) {
  // make new states, one per block, and map old ids to new ids
  StateToStateMap oldToNew;
  StateId errId = outDfa.newState();
  StateId initId = outDfa.newState();
  if ((errId != gDfaErrorId) || (initId != gDfaInitialId))
    throw RedExcept("dfa state ids not what was expected");
  oldToNew.emplace(gDfaErrorId, gDfaErrorId);
  oldToNew.emplace(gDfaInitialId, gDfaInitialId);

  for (const StateIdSet &sis : blocks) {
    auto it = sis.begin();
    if (it != sis.end()) {
      StateId id;
      auto found = oldToNew.find(*it);
      if (found == oldToNew.end())
        id = outDfa.newState();
      else
        id = found->second;
      for (; it != sis.end(); ++it)
        oldToNew.emplace(*it, id); // FIXME: was insert_or_assign
    }
  }

  // transcribe state transitions to new ids
  for (const StateIdSet &sis : blocks) {
    auto it = sis.begin();
    if (it != sis.end()) {
      const DfaState &srcState = srcDfa[*it];
      DfaState &outState = outDfa[oldToNew[*it]];
      for (auto srcIt = srcState.trans_.getMap().cbegin();
           srcIt != srcState.trans_.getMap().cend();
           ++srcIt)
        outState.trans_.emplace(srcIt->first, oldToNew[srcIt->second]);
      outState.result_ = srcState.result_;
      outState.deadEnd_ = srcState.deadEnd_;
    }
  }
}


void improveDfa(DfaObj &dfa) {
  flagDeadEnds(dfa.getMutStates());
  chopEndMarks(dfa.getMutStates());
  // std::cout << "FIXME post chop " << toString(dfa) << std::endl;

  {
    DfaObj small = transcribeDfa(dfa);
    dfa.swap(small);
  }

  //groupInputs();
  //compressDfa(); // FIXME: move to "final compilation"
}

///////////////////////////////////////////////////////////////////////////////

void DfaMinimizer::minimize() {
  DfaObj work;

  setup();
  iterate();
  cleanup(work);

  src_.swap(work);
}


void DfaMinimizer::setup() {
  maxChar_ = findMaxChar(src_.getStates());

  pullBackEndMarks(src_.getMutStates());

  {
    StateIdSet stateSet = src_.allStateIds();

    inverse_ = std::move(invert(stateSet, src_.getStates(), maxChar_));

    blocks_.clear();
    blocks_.resize(2);
    partition(stateSet, src_.getStates(),
              blocks_[gNormalBlock], blocks_[gAcceptBlock]);
  }

  list_ = std::move(makeList(maxChar_,
                             blocks_[gNormalBlock], blocks_[gAcceptBlock]));
}


void DfaMinimizer::iterate() {
  vector<BlockId> twins;
  PatchSet patches;

  // std::cout << "FIXME blocks " << toString(blocks_) << std::endl;
  while (!list_.empty()) {
    auto node = list_.extract(list_.begin());
    BlockRec &br = node.value();
    StateIdSet splits = locateSplits(br, blocks_, inverse_);
    if (splits.population() > 0) {
      // std::cout << "FIXME rec " << toString(br) << std::endl;
      // std::cout << "FIXME splits " << toString(splits) << std::endl;
      performSplits(br, splits, twins, patches, src_.getStates(), blocks_);
      patchBlocks(patches, list_, maxChar_, blocks_);
      // std::cout << "FIXME blocks " << toString(blocks_) << std::endl;
    }
  }

  inverse_.clear();
}


void DfaMinimizer::cleanup(DfaObj &work) {
  finalizeDfa(src_, work, blocks_);
  blocks_.clear();
  blocks_.shrink_to_fit();
  improveDfa(work);
  maxChar_ = 0;
}

} // namespace zezax::red
