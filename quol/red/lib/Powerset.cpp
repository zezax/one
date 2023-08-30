/* Powerset.cpp - powerset NFA to DFA converter implementation

   See general description in Powerset.h

   The approach taken here is akin to this table-based description:
   https://www.geeksforgeeks.org/conversion-from-nfa-to-dfa/
   https://quickgrid.wordpress.com/2015/10/30/converting-nfa-to-dfa-by-complete-and-lazy-subset-construction/
   As it turns out, makeTable() takes the lion's share of the time.

   An important input to makeTable() is the set of "basis"
   multi-chars.  These partition the set of all in-use characters
   such that within each partition, all characters behave the same
   in the entire NFA.  Each character could be its own partition,
   but The smallest number of partitions is most efficient.

   The actual conversion happens in dfaFromNfaRecurse() which is
   relatively straightforward because it relies on the previously-
   done work represented in the table and the basis set.

   The convert() method is the main flow, and removes end marks
   before returning.
 */

#include <algorithm>
#include <deque>
#include <limits>
#include <unordered_map>

#include "Except.h"
#include "Util.h"
#include "Powerset.h"

namespace zezax::red {

using std::deque;
using std::numeric_limits;
using std::unordered_map;
using std::vector;

namespace {

Result getResult(const NfaIdSet     &nis,
                 const NfaIdToCount &counts,
                 const NfaObj       &nfa) {
  Result rv = -1;
  size_t min = numeric_limits<size_t>::max();

  for (NfaId id : nis) {
    auto it = counts.find(id);
    if (it != counts.end()) {
      size_t num = it->second;
      if (num < min) { // lowest number of accepting wins
        min = num;
        rv = nfa[id].result_;
      }
    }
  }

  return rv;
}

} // anonymous

///////////////////////////////////////////////////////////////////////////////

DfaObj PowersetConverter::convert() {
  if (stats_)
    stats_->preDfa_ = std::chrono::steady_clock::now();

  NfaId initial = nfa_.getInitial();

  vector<MultiChar> multiChars;
  {
    MultiCharSet allMcs = nfa_.allMultiChars(initial);
    MultiCharSet basisMcs = basisMultiChars(allMcs);
    allMcs.clear(); // save memory
    for (const MultiChar &mc : basisMcs)
      multiChars.emplace_back(mc); // copy here -> makeTable() faster
  }

  if (stats_)
    stats_->postBasisChars_ = std::chrono::steady_clock::now();

  NfaStatesToTransitions table = makeTable(initial, nfa_, multiChars);

  if (stats_)
    stats_->postMakeTable_ = std::chrono::steady_clock::now();

  NfaIdToCount counts = countAcceptingStates(table, nfa_);

  DfaObj dfa(budget_);
  DfaId id = dfa.newState();
  if (id != gDfaErrorId)
    throw RedExceptCompile("dfa error state must be zero");

  NfaIdSet states;
  states.insert(initial);
  auto it = table.find(states);
  if (it == table.end())
    throw RedExceptCompile("cannot find initial nfa states");
  id = dfaFromNfa(multiChars, table, counts, states, dfa);
  if (id != gDfaInitialId)
    throw RedExceptCompile("dfa initial state must be one");

  if (stats_)
    stats_->powersetMemUsed_ = bytesUsed();

  dfa.chopEndMarks(); // end marks have done their job

  if (stats_) {
    stats_->origDfaStates_       = dfa.numStates();
    stats_->transitionTableRows_ = table.size();
    stats_->postDfa_             = std::chrono::steady_clock::now();
  }
  return dfa;
}


DfaId PowersetConverter::dfaFromNfa(const std::vector<MultiChar> &multiChars,
                                    const NfaStatesToTransitions &table,
                                    const NfaIdToCount           &counts,
                                    const NfaIdSet               &states,
                                    DfaObj                       &dfa) {
  NfaStatesToId map;
  return dfaFromNfaRecurse(multiChars, table, counts, states, map, nfa_, dfa);
}

///////////////////////////////////////////////////////////////////////////////

// returns a minimal set of multi-chars that partitions the input
MultiCharSet basisMultiChars(const MultiCharSet &mcs) {
  typedef BitSet<size_t, DefaultTag> SeqSet;
  typedef unordered_map<CharIdx, SeqSet> SeqSetMap;
  typedef vector<std::pair<SeqSet, CharIdx>> Inverted;

  Inverted inv;

  { // for each character, gather all multi-chars it's part of
    SeqSetMap appear;
    size_t seq = 0;
    for (const MultiChar &mc : mcs) {
      for (CharIdx ch : mc)
        appear[ch].insert(seq);
      ++seq;
    }

    // invert the above into an array to be sorted
    for (auto &[ch, seqs] : appear)
      inv.emplace_back(std::move(seqs), ch);
  }

  std::sort(inv.begin(), inv.end());

  // extract groups that appear in the exact same places
  SeqSet       last;
  MultiChar    acc;
  MultiCharSet rv;
  for (auto &[seqs, ch] : inv) {
    if (seqs != last) {
      if (!acc.empty()) {
        rv.emplace(std::move(acc));
        acc.clearAll();
      }
      last = std::move(seqs);
    }
    acc.insert(ch);
  }
  if (!acc.empty())
    rv.emplace(std::move(acc));

  return rv;
}


// this is a performace-critical function
NfaStatesToTransitions makeTable(NfaId                    initial,
                                 const NfaObj            &nfa,
                                 const vector<MultiChar> &allMultiChars) {
  NfaStatesToTransitions table;
  typedef NfaStatesToTransitions::iterator Placeholder;

  const size_t allSize = allMultiChars.size();

  NfaIdSet initialStates;
  initialStates.insert(initial);
  std::pair<NfaIdSet, IdxToNfaIdSet> tableNode;
  tableNode.first = std::move(initialStates);
  auto [iter, dummy] = table.emplace(std::move(tableNode));
  deque<Placeholder> todoList;
  todoList.emplace_back(iter);

  while (!todoList.empty()) {
    Placeholder tableIt = todoList.back();
    todoList.pop_back();

    // iterate bit-set once, as it's more expensive to do
    for (NfaId id : tableIt->first)
      for (const NfaTransition &trans : nfa[id].transitions_)
        for (size_t idx = 0; idx < allSize; ++idx)
          if (allMultiChars[idx].hasIntersection(trans.multiChar_))
            tableIt->second[idx].insert(trans.next_);
    for (const auto &[_, nis] : tableIt->second) {
      std::pair<NfaIdSet, IdxToNfaIdSet> tNode;
      tNode.first = nis;
      auto [it, novel] = table.emplace(std::move(tNode));
      if (novel)
        todoList.emplace_back(it);
    }
  }

  return table;
}


NfaIdToCount countAcceptingStates(const NfaStatesToTransitions &table,
                                  const NfaObj                 &nfa) {
  NfaIdToCount rv;
  for (const auto &[states, _] : table)
    for (NfaId id : states)
      if (nfa.accepts(id)) {
        auto [it, dummy] = rv.emplace(id, 0);
        ++it->second;
      }
  return rv;
}


DfaId dfaFromNfaRecurse(const vector<MultiChar>      &multiChars,
                        const NfaStatesToTransitions &table,
                        const NfaIdToCount           &counts,
                        const NfaIdSet               &stateSet,
                        NfaStatesToId                &map,
                        const NfaObj                 &nfa,
                        DfaObj                       &dfa) {
  std::pair<NfaIdSet, DfaId> mapNode;
  mapNode.first = stateSet;
  auto [mapIter, novel] = map.emplace(std::move(mapNode));
  if (!novel)
    return mapIter->second;

  DfaId dfaId = dfa.newState();
  mapIter->second = dfaId;

  auto tableIter = table.find(stateSet);
  if (tableIter == table.end())
    return dfaId; // FIXME: can this happen? is it right?

  for (const auto &[ii, nis] : tableIter->second) {
    if (!nis.empty()) {
      DfaId subId = dfaFromNfaRecurse(
          multiChars, table, counts, nis, map, nfa, dfa);
      for (CharIdx ch : multiChars[ii]) // FIXME: why is mc[ii] valid?
        dfa[dfaId].transitions_.set(ch, subId);
    }
  }

  if (nfa.hasAccept(stateSet))
    dfa[dfaId].result_ = getResult(stateSet, counts, nfa);

  return dfaId;
}

} // namespace zezax::red
