/* Minimizer.cpp - DFA minimizer implementation

   See general description in Minimizer.h

   The worst-case cost of minimization is proportional to the size
   of the alphabet.  So, this implementation uses the initial DFA
   to find clusters of tokens that are equivalent.  These clusters
   then become the new reduced alphabet.  The minimized DFA has an
   equivalence map from real to reduced alphabet.

   The basic flow here is probably best followed by reading the Gries
   paper.  The idea is to build blocks which are sets of DFA state IDs
   from the source DFA.  Blocks are successively split.  Eventually,
   each block becomes a state in the output DFA.

   The invert() operation speeds up the splitting process by
   precomputing the inverse directed graph from edges to originating
   states.

   An initial partition is created based on the result of each state.
   All non-accepting states are one block.  Accepting states are
   in blocks based on the value of their result.

   The algorithm begins with either the accepting or non-accepting
   preliminary blocks added to a set of blocks to be processed
   (called the "list").  The main loop runs until the list is empty.
   It tries to split each block.  If posible, the resulting blocks
   may be added to the list.  Often only the smaller block must be
   added.  Eventually, all blocks will be handled.

   After minimization, dead-end states are flagged.  These are states
   which cannot be escaped regardless of input.  Thus the DFA result
   can't change and no further processing of input is warranted.
 */

#include "Minimizer.h"

#include <map>

#include "Except.h"
#include "Util.h"
#include "Dfa.h"

namespace zezax::red {

using std::vector;

namespace {

DfaId &twinRef(vector<BlockId> &vec, size_t idx) {
  while (vec.size() <= idx)
    vec.push_back(-1);
  return vec[idx];
}


BlockId findInBlock(DfaId id, const vector<DfaIdSet> &blocks) {
  DfaId block = 0;
  for (const DfaIdSet &dis : blocks) {
    if (dis.get(id))
      return block;
    ++block;
  }
  throw RedExceptMinimize("no block containing state");
}

} // anonymous

///////////////////////////////////////////////////////////////////////////////

void DfaMinimizer::minimize() {
  if (stats_)
    stats_->preMinimize_ = std::chrono::steady_clock::now();

  setup();
  iterate();

  DfaObj work(src_.getBudget());
  cleanup(work);
  src_.swap(work);

  if (stats_) {
    stats_->minimizedDfaStates_      = src_.numStates();
    stats_->numDistinguishedSymbols_ = maxChar_ + 1;
    stats_->postMinimize_            = std::chrono::steady_clock::now();
  }
}


void DfaMinimizer::setup() {
  maxChar_ = src_.installEquivalenceMap(); // smaller alphabet means less work
  if (stats_)
    stats_->postEquivMap_ = std::chrono::steady_clock::now();

  {
    DfaIdSet stateSet = src_.allStateIds();

    inverse_ = invert(stateSet, src_.getStates(), maxChar_);
    if (stats_)
      stats_->postInvert_ = std::chrono::steady_clock::now();

    blocks_.clear();
    partition(stateSet, src_.getStates(), blocks_);
    if (stats_)
      stats_->postPartition_ = std::chrono::steady_clock::now();
  }

  list_ = makeList(maxChar_, blocks_);
  if (stats_)
    stats_->postMakeList_ = std::chrono::steady_clock::now();
}


void DfaMinimizer::iterate() {
  vector<BlockId> twins;
  PatchSet patches;

  while (!list_.empty()) {
    auto node = list_.extract(list_.begin());
    BlockRec &br = node.value();
    DfaIdSet splits = locateSplits(br, blocks_, inverse_);
    if (!splits.empty()) {
      performSplits(br, splits, twins, patches, src_.getStates(), blocks_);
      patchBlocks(patches, list_, maxChar_, blocks_);
    }
  }

  inverse_.clear();
}


void DfaMinimizer::cleanup(DfaObj &work) {
  makeDfaFromBlocks(src_, work, blocks_);
  blocks_.clear();
  blocks_.shrink_to_fit(); // free some memory
  work.copyEquivMap(src_);
  flagDeadEnds(work.getMutStates(), maxChar_);
}

///////////////////////////////////////////////////////////////////////////////

DfaEdgeToIds invert(const DfaIdSet         &stateSet,
                    const vector<DfaState> &stateVec,
                    CharIdx                 maxChar) {
  DfaEdgeToIds rv;

  for (DfaId did : stateSet) {
    const DfaState &ds = stateVec[did];
    for (CharIdx ch = 0; ch <= maxChar; ++ch) { // need to enumerate all
      std::pair<DfaEdge, DfaIdSet> node;
      node.first.id_ = ds.transitions_[ch];
      node.first.char_ = ch;
      auto [it, _] = rv.emplace(std::move(node));
      it->second.insert(did);
    }
  }

  return rv;
}


// Create initial partition of blocks by result
void partition(const DfaIdSet         &stateSet,
               const vector<DfaState> &stateVec,
               vector<DfaIdSet>       &blockVec) {
  // states with different results must be differentiated into different blocks
  std::map<Result, BlockId> result2block;

  {
    ResultSet resultSet;
    resultSet.insert(0); // make sure non-accepting result is present
    for (DfaId did : stateSet)
      resultSet.insert(stateVec[did].result_);

    BlockId block = 0;
    for (Result res : resultSet)
      result2block[res] = block++;
  }

  for (DfaId did : stateSet) {
    BlockId block = result2block[stateVec[did].result_];
    safeRef(blockVec, block).insert(did);
  }
}


// Create the initial work list: either rejecting block or all accepting
BlockRecSet makeList(CharIdx                 maxChar,
                     const vector<DfaIdSet> &blocks) {
  DfaId zeroSize = blocks[0].size();
  DfaId restSize = 0;

  BlockId num = static_cast<BlockId>(blocks.size());
  for (BlockId bid = 1; bid < num; ++bid)
    restSize += blocks[bid].size();

  BlockId start = 0;
  BlockId end = 1;
  if ((restSize > 0) && (restSize < zeroSize)) {
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
                   vector<DfaIdSet>       &blockVec) {
  twins.clear(); // forget old
  for (DfaId splitId : splits) {
    BlockId blockNum = findInBlock(splitId, blockVec);
    if (!containedIn(blockNum, blockRec.block_, blockRec.char_,
                     stateVec, blockVec))
      handleTwins(splitId, blockNum, blockVec, twins, patches);
  }
}


bool containedIn(BlockId                 needleId,
                 BlockId                 haystackId,
                 CharIdx                 ch,
                 const vector<DfaState> &stateVec,
                 const vector<DfaIdSet> &blockVec) {
  const DfaIdSet &needle   = blockVec[needleId];
  const DfaIdSet &haystack = blockVec[haystackId];

  for (DfaId id : needle) {
    const DfaState &ds = stateVec[id];
    if (!haystack.get(ds.transitions_[ch]))
      return false;
  }

  return true;
}


// While splitting, move state to appropriate twin and record patch
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
  blocks[twin].insert(stateId);
  patches.emplace(Patch{blockId, twin});
}


void patchBlocks(PatchSet               &patches,
                 BlockRecSet            &list,
                 CharIdx                 maxChar,
                 const vector<DfaIdSet> &blocks) {
  for (const Patch &patch : patches)
    patchPair(patch.block_, patch.twin_, list, maxChar, blocks);
  patches.clear();
}


void patchPair(BlockId                 ii,
               BlockId                 jj,
               BlockRecSet            &list,
               CharIdx                 maxChar,
               const vector<DfaIdSet> &blocks) {
  BlockRec bii;
  BlockRec bjj;
  bii.block_ = ii;
  bjj.block_ = jj;
  BlockRec bmin;
  bmin.block_ = (blocks[ii].size() < blocks[jj].size()) ? ii : jj;

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
  DfaId errId  = outDfa.newState();
  DfaId initId = outDfa.newState();
  if ((errId != gDfaErrorId) || (initId != gDfaInitialId))
    throw RedExceptMinimize("dfa state ids not what was expected");
  oldToNew.emplace(gDfaErrorId,   errId);
  oldToNew.emplace(gDfaInitialId, initId);

  for (const DfaIdSet &dis : blocks) {
    auto it = dis.begin();
    if (it != dis.end()) {
      DfaId id;
      auto found = oldToNew.find(*it);
      if (found == oldToNew.end())
        id = outDfa.newState();
      else
        id = found->second;
      for (; it != dis.end(); ++it)
        oldToNew.emplace(*it, id);
    }
  }

  // transcribe state transitions to new ids
  for (const DfaIdSet &dis : blocks) {
    auto it = dis.begin();
    if (it != dis.end()) {
      const DfaState &srcState = srcDfa[*it];
      DfaState &outState = outDfa[oldToNew[*it]];
      for (auto [ch, st] : srcState.transitions_.getMap())
        outState.transitions_.emplace(ch, oldToNew[st]);
      outState.result_  = srcState.result_;
      outState.deadEnd_ = srcState.deadEnd_;
    }
  }
}

} // namespace zezax::red
