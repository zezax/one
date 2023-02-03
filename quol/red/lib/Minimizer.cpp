// dfa minimizer implementation after david gries 1973

// Note: This is used for DFAs with end marks.  Untested without.

#include "Minimizer.h"

#include <cassert>
#include <map>

#include "Except.h"
#include "Util.h"
#include "Dfa.h"

namespace zezax::red {

using std::vector;

namespace {

DfaId &twinRef(vector<DfaId> &vec, size_t idx) {
  while (vec.size() <= idx)
    vec.push_back(-1);
  return vec[idx];
}


BlockId findInBlock(DfaId id, const vector<DfaIdSet> &blocks) {
  for (BlockId block = 0; static_cast<size_t>(block) < blocks.size(); ++block)
    if (blocks[block].get(id))
      return block;
  throw RedExceptMinimize("no block containing state");
}

} // anonymous

DfaEdgeToIds invert(const DfaIdSet         &stateSet,
                    const vector<DfaState> &stateVec,
                    CharIdx                 maxChar) {
  DfaEdgeToIds rv;

  for (DfaId did : stateSet) {
    const DfaState &ds = stateVec[did];
    for (CharIdx ch = 0; ch <= maxChar; ++ch) { // need to enumerate all
      std::pair<DfaEdge, DfaIdSet> node;
      node.first.id_ = ds.trans_[ch];
      node.first.char_ = ch;
      auto [it, _] = rv.emplace(std::move(node));
      it->second.set(did);
    }
  }

  return rv;
}


void partition(const DfaIdSet         &stateSet,
               const vector<DfaState> &stateVec,
               vector<DfaIdSet>       &blocks) {
  ResultSet resultSet;
  resultSet.set(0); // make sure non-accepting result is present
  for (DfaId did : stateSet)
    resultSet.set(stateVec[did].result_);

  // states with different results must be differentiated into different blocks
  std::map<Result, BlockId> result2block;
  BlockId block = 0;
  for (Result res : resultSet)
    result2block[res] = block++;

  for (DfaId did : stateSet) {
    block = result2block[stateVec[did].result_];
    safeRef(blocks, block).set(did);
  }
}


BlockRecSet makeList(CharIdx                 maxChar,
                     const vector<DfaIdSet> &blocks) {
  DfaId zeroPop = blocks[0].population();
  DfaId restPop = 0;

  BlockId num = static_cast<BlockId>(blocks.size());
  for (BlockId bid = 1; bid < num; ++bid)
    restPop += blocks[bid].population();

  BlockId start = 0;
  BlockId end = 1;
  if ((restPop > 0) && (restPop < zeroPop)) {
    start = 1;
    end = num;
  }

  BlockRecSet list;
  for (BlockId bid = start; bid < end; ++bid) {
    BlockRec br;
    br.block_ = bid;
    for (CharIdx ch = 0; ch <= maxChar; ++ch) {
      br.char_ = ch;
      list.insert(br);
    }
  }

  return list;
}


DfaIdSet locateSplits(const BlockRec         &blockRec,
                      const vector<DfaIdSet> &blocks,
                      const DfaEdgeToIds     &inv) {
  DfaIdSet rv;
  DfaEdge key;
  key.char_ = blockRec.char_;
  for (DfaId id : blocks[blockRec.block_]) {
    key.id_ = id;
    auto it = inv.find(key);
    if (it != inv.end())
      rv.unionWith(it->second);
  }
  return rv;
}


void performSplits(const BlockRec         &blockRec,
                   const DfaIdSet         &splits,
                   vector<BlockId>        &twins,
                   PatchSet               &patches,
                   const vector<DfaState> &stateVec,
                   vector<DfaIdSet>       &blocks) {
  twins.clear(); // forget old
  for (DfaId splitId : splits) {
    BlockId blockNum = findInBlock(splitId, blocks);
    if (!containedIn(blockNum, blockRec.block_, blockRec.char_,
                     stateVec, blocks))
      handleTwins(splitId, blockNum, blocks, twins, patches);
  }
}


bool containedIn(BlockId                 needleId,
                 BlockId                 haystackId,
                 CharIdx                 ch,
                 const vector<DfaState> &stateVec,
                 const vector<DfaIdSet> &blocks) {
  const DfaIdSet &needle = blocks[needleId];
  const DfaIdSet &haystack = blocks[haystackId];

  for (DfaId id : needle) {
    const DfaState &ds = stateVec[id];
    if (!haystack.get(ds.trans_[ch]))
      return false;
  }

  return true;
}


void handleTwins(DfaId             stateId,
                 BlockId           blockId,
                 vector<DfaIdSet> &blocks,
                 vector<BlockId>  &twins,
                 PatchSet         &patches) {
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


void patchBlocks(PatchSet               &patches,
                 BlockRecSet            &list,
                 CharIdx                 maxChar,
                 const vector<DfaIdSet> &blocks) {
  for (const Patch &patch : patches)
    patchPair(patch.first, patch.second, list, maxChar, blocks);
  patches.clear();
}


void patchPair(BlockId                 ii,
               BlockId                 jj,
               BlockRecSet            &list,
               CharIdx                 maxChar,
               const vector<DfaIdSet> &blocks) {
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


void makeDfaFromBlocks(const DfaObj           &srcDfa,
                       DfaObj                 &outDfa,
                       const vector<DfaIdSet> &blocks) {
  // make new states, one per block, and map old ids to new ids
  StateToStateMap oldToNew;
  DfaId errId = outDfa.newState();
  DfaId initId = outDfa.newState();
  if ((errId != gDfaErrorId) || (initId != gDfaInitialId))
    throw RedExceptMinimize("dfa state ids not what was expected");
  oldToNew.emplace(gDfaErrorId, gDfaErrorId);
  oldToNew.emplace(gDfaInitialId, gDfaInitialId);

  for (const DfaIdSet &sis : blocks) {
    auto it = sis.begin();
    if (it != sis.end()) {
      DfaId id;
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
  for (const DfaIdSet &dis : blocks) {
    auto it = dis.begin();
    if (it != dis.end()) {
      const DfaState &srcState = srcDfa[*it];
      DfaState &outState = outDfa[oldToNew[*it]];
      for (auto [ch, st] : srcState.trans_.getMap())
        outState.trans_.emplace(ch, oldToNew[st]);
      outState.result_ = srcState.result_;
      outState.deadEnd_ = srcState.deadEnd_;
    }
  }
}


void improveDfa(DfaObj &dfa, CharIdx maxChar) {
  flagDeadEnds(dfa.getMutStates(), maxChar);

  {
    DfaObj small = transcribeDfa(dfa);
    dfa.swap(small);
  }
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
  src_.installEquivalenceMap(); // smaller alphabet means less work
  maxChar_ = src_.findMaxChar();

  {
    DfaIdSet stateSet = src_.allStateIds();

    inverse_ = std::move(invert(stateSet, src_.getStates(), maxChar_));

    blocks_.clear();
    partition(stateSet, src_.getStates(), blocks_);
  }

  list_ = std::move(makeList(maxChar_, blocks_));
}


void DfaMinimizer::iterate() {
  vector<BlockId> twins;
  PatchSet patches;

  while (!list_.empty()) {
    auto node = list_.extract(list_.begin());
    BlockRec &br = node.value();
    DfaIdSet splits = locateSplits(br, blocks_, inverse_);
    if (splits.population() > 0) {
      performSplits(br, splits, twins, patches, src_.getStates(), blocks_);
      patchBlocks(patches, list_, maxChar_, blocks_);
    }
  }

  inverse_.clear();
}


void DfaMinimizer::cleanup(DfaObj &work) {
  makeDfaFromBlocks(src_, work, blocks_);
  blocks_.clear();
  blocks_.shrink_to_fit();
  work.copyEquivMap(src_);
  improveDfa(work, maxChar_);
  maxChar_ = 0;
}

} // namespace zezax::red
