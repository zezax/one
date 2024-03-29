/* Minimizer.h - DFA minimizer header

   DfaMinimizer converts a deterministic finite automaton (DFA) into
   another DFA which is the smallest possible with the same results.
   Some background on this classic bit of computer science is here:
   https://en.wikipedia.org/wiki/DFA_minimization

   The DfaMinimizer interface appears to minimize the DFA in-place,
   but really it's creating a new one and performing a swap.

   The implementation here follows the 1973 David Gries paper:
   Describing an Algorithm by Hopcroft.  Link here:
   https://www.cs.cornell.edu/gries/TechReports/72-151.pdf
   The basic algorithm is Hopcroft's partition refinement from 1971.
   Runtime averages O(N log log N) and may reach O(NM log N), where
   N = number of states
   M = size of alphabet
   Reductions of 14% in number of states have been observed.

   The Budget passed to Parser is honored here.  A CompStats pointer
   may also be passed to DfaMinimizer if desired.

   Usage is simple:

   DfaMinimizer dm(dfa, stats);
   dm.minimize();

   DfaMinimizer will throw RedExcept exceptions for internal errors.
 */

#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Fnv.h"
#include "Dfa.h"

namespace zezax::red {

// BlockId is actually an index into the array of blocks
typedef int64_t BlockId;

// DfaEdge represents a transition to a specific DFA state ID due to
// encountering a specific input character.  DfaMinimizer incorporates
// DfaEdge elements into an inverse of the original DFA, for working
// backward to a set of states.  This makes splitting blocks efficient.
struct DfaEdge {
  DfaId   id_;
  CharIdx char_;
};

// BlockRec represents a work item in the "list".  The idea is to split
// the block with respect to the character.
struct BlockRec {
  BlockId block_;
  CharIdx char_;
};

// Patch represents the fix-up work after splitting a block.  The Gries
// paper explains the twin.  Basically, the twin is the block to which
// states are moved after removing them from the block.
struct Patch {
  BlockId block_;
  BlockId twin_;
};

// DfaEdgeToIds represents the inverse DFA used for splitting
typedef std::unordered_map<DfaEdge, DfaIdSet> DfaEdgeToIds;

// BlockRecSet is the "list" of work to be done
typedef std::unordered_set<BlockRec>          BlockRecSet;

// PatchSet holds the fix-up work to be done after each iteration
typedef std::unordered_set<Patch>             PatchSet;

} // namespace zezax::red


// A brief interlude in the top-level namespace so that we can use various
// structs defined above in std::unordered_map and std::unordered_set.

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


template<> struct std::equal_to<zezax::red::Patch> {
  bool operator()(const zezax::red::Patch &aa,
                  const zezax::red::Patch &bb) const {
    return ((aa.block_ == bb.block_) && (aa.twin_ == bb.twin_));
  }
};


template<> struct std::hash<zezax::red::Patch> {
  size_t operator()(const zezax::red::Patch &p) const {
    size_t h = zezax::red::fnv1a<size_t>(&p.block_, sizeof(p.block_));
    return zezax::red::fnv1aInc<size_t>(h, &p.twin_, sizeof(p.twin_));
  }
};

namespace zezax::red {

// This is what you're looking for
class DfaMinimizer {
public:
  explicit DfaMinimizer(DfaObj &dfa, CompStats *stats = nullptr)
    : src_(dfa), stats_(stats) {} // dfa will be modified

  void minimize();

private:
  void setup();
  void iterate();
  void cleanup(DfaObj &work);

  DfaObj               &src_;
  CharIdx               maxChar_;
  DfaEdgeToIds          inverse_;
  std::vector<DfaIdSet> blocks_;
  BlockRecSet           list_;
  CompStats            *stats_;
};


// constituent functions, public for unit tests
DfaEdgeToIds invert(const DfaIdSet              &stateSet,
                    const std::vector<DfaState> &stateVec,
                    CharIdx                      maxChar);

void partition(const DfaIdSet              &stateSet,
               const std::vector<DfaState> &stateVec,
               std::vector<DfaIdSet>       &blockVec);

BlockRecSet makeList(CharIdx                      maxChar,
                     const std::vector<DfaIdSet> &blocks);

DfaIdSet locateSplits(const BlockRec                &blockRec,
                        const std::vector<DfaIdSet> &blocks,
                        const DfaEdgeToIds          &inv);

void performSplits(const BlockRec              &blockRec,
                   const DfaIdSet              &splits,
                   std::vector<BlockId>        &twins,
                   PatchSet                    &patches,
                   const std::vector<DfaState> &stateVec,
                   std::vector<DfaIdSet>       &blockVec);

bool containedIn(BlockId                      needleId,
                 BlockId                      haystackId,
                 CharIdx                      ch,
                 const std::vector<DfaState> &stateVec,
                 const std::vector<DfaIdSet> &blockVec);

void handleTwins(DfaId                  stateId,
                 BlockId                blockId,
                 std::vector<DfaIdSet> &blocks,
                 std::vector<DfaId>    &twins,
                 PatchSet              &patches);

void patchBlocks(PatchSet                    &patches,
                 BlockRecSet                 &list,
                 CharIdx                      maxChar,
                 const std::vector<DfaIdSet> &blocks);

void patchPair(BlockId                      ii,
               BlockId                      jj,
               BlockRecSet                 &list,
               CharIdx                      maxChar,
               const std::vector<DfaIdSet> &blocks);

void makeDfaFromBlocks(const DfaObj                &srcDfa,
                       DfaObj                      &outDfa,
                       const std::vector<DfaIdSet> &blocks);

} // namespace zezax::red
