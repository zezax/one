// dfa minimizer header

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Fnv.h"
#include "Dfa.h"

namespace zezax::red {

typedef int64_t BlockId;

struct DfaEdge {
  StateId id_;
  CharIdx char_;
};

struct BlockRec {
  BlockId block_;
  CharIdx char_;
};

typedef std::unordered_set<DfaEdge> DfaEdgeSet;
typedef std::unordered_map<DfaEdge, StateIdSet> DfaEdgeToIds;
typedef std::unordered_set<BlockRec> BlockRecSet;
typedef std::pair<BlockId, BlockId> Patch;
typedef std::unordered_set<Patch> PatchSet;

} // namespace zezax::red

// for use in std::unordered map
template<> struct std::equal_to<zezax::red::DfaEdge> {
  bool operator()(const zezax::red::DfaEdge &aa,
                  const zezax::red::DfaEdge &bb) const {
    return ((aa.id_ == bb.id_) && (aa.char_ == bb.char_));
  }
};


template<> struct std::hash<zezax::red::DfaEdge> {
  size_t operator()(const zezax::red::DfaEdge &de) const {
    size_t h = zezax::red::fnv1a<size_t>(&de.id_, sizeof(de.id_));
    return zezax::red::fnv1aInc<size_t>(h, &de.char_, sizeof(de.char_));
  }
};


template<> struct std::equal_to<zezax::red::BlockRec> {
  bool operator()(const zezax::red::BlockRec &aa,
                  const zezax::red::BlockRec &bb) const {
    return ((aa.block_ == bb.block_) && (aa.char_ == bb.char_));
  }
};


template<> struct std::hash<zezax::red::BlockRec> {
  size_t operator()(const zezax::red::BlockRec &br) const {
    size_t h = zezax::red::fnv1a<size_t>(&br.block_, sizeof(br.block_));
    return zezax::red::fnv1aInc<size_t>(h, &br.char_, sizeof(br.char_));
  }
};


template<> struct std::hash<zezax::red::Patch> {
  size_t operator()(const zezax::red::Patch &p) const {
    size_t h = zezax::red::fnv1a<size_t>(&p.first, sizeof(p.first));
    return zezax::red::fnv1aInc<size_t>(h, &p.second, sizeof(p.second));
  }
};

namespace zezax::red {

class DfaMinimizer {
public:
  DfaMinimizer(DfaObj &dfa) : src_(dfa) {} // dfa will be modified

  void minimize();

private:
  void setup();
  void iterate();
  void cleanup(DfaObj &work);

  DfaObj                 &src_;
  CharIdx                 maxChar_;
  DfaEdgeToIds            inverse_;
  std::vector<StateIdSet> blocks_;
  BlockRecSet             list_;
};


// building blocks
CharIdx findMaxChar(const std::vector<DfaState> &stateVec);

void pullBackEndMarks(std::vector<DfaState> &states);

void chopEndMarks(std::vector<DfaState> &states);

DfaEdgeToIds invert(const StateIdSet            &stateSet,
                    const std::vector<DfaState> &stateVec,
                    CharIdx                      maxChar);

void partition(const StateIdSet            &stateSet,
               const std::vector<DfaState> &stateVec,
               StateIdSet                  &normal,
               StateIdSet                  &accept);

BlockRecSet makeList(CharIdx           maxChar,
                     const StateIdSet &normal,
                     const StateIdSet &accept);

StateIdSet locateSplits(const BlockRec                &blockRec,
                        const std::vector<StateIdSet> &blocks,
                        const DfaEdgeToIds            &inv);

void performSplits(const BlockRec              &blockRec,
                   const StateIdSet            &splits,
                   std::vector<BlockId>        &twins,
                   PatchSet                    &patches,
                   const std::vector<DfaState> &stateVec,
                   std::vector<StateIdSet>     &blocks);

bool containedIn(BlockId                        needleId,
                 BlockId                        haystackId,
                 CharIdx                        ch,
                 const std::vector<DfaState>   &stateVec,
                 const std::vector<StateIdSet> &blocks);

void handleTwins(StateId                  src,
                 BlockId                  dst,
                 std::vector<StateIdSet> &blocks,
                 std::vector<StateId>    &twins,
                 PatchSet                &patches);

void patchBlocks(PatchSet                      &patches,
                 BlockRecSet                   &list,
                 CharIdx                        maxChar,
                 const std::vector<StateIdSet> &blocks);

void patchPair(BlockId                        ii,
               BlockId                        jj,
               BlockRecSet                   &list,
               CharIdx                        maxChar,
               const std::vector<StateIdSet> &blocks);

void finalize(const DfaObj &srcDfa,
              DfaObj &outDfa,
              const std::vector<StateIdSet> &blocks);

void improveDfa(DfaObj &dfa);

} // namespace zezax::red
