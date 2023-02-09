// powerset nfa to dfa converter implementation
// Rabin-Scott method

#include <limits>

#include "Except.h"
#include "Util.h"
#include "Powerset.h"

namespace zezax::red {

using std::numeric_limits;
using std::unordered_set;
using std::vector;

namespace {

Result getResult(const NfaIdSet        &nis,
                 const NfaStateToCount &counts,
                 const NfaObj          &nfa) {
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

  NfaId initial = nfa_.getNfaInitial();

  vector<MultiChar> multiChars;
  {
    MultiCharSet allMcs = nfa_.allMultiChars(initial);
    MultiCharSet basisMcs = basisMultiChars(allMcs);
    allMcs.clear(); // save memory
    for (const MultiChar &mc : basisMcs)
      multiChars.emplace_back(std::move(mc));
  }

  NfaStatesToTransitions table = makeTable(initial, nfa_, multiChars);
  NfaStateToCount counts = countAcceptingStates(table, nfa_);

  DfaObj dfa;
  DfaId id = dfa.newState();
  if (id != gDfaErrorId)
    throw RedExceptCompile("dfa error state must be zero");

  NfaIdSet states;
  states.set(initial);
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
    DfaIdSet all = dfa.allStateIds();
    stats_->origDfaStates_ = dfa.numStates();
    stats_->transitionTableRows_ = table.size();
    stats_->postDfa_ = std::chrono::steady_clock::now();
  }
  return dfa;
}


DfaId PowersetConverter::dfaFromNfa(const std::vector<MultiChar> &multiChars,
                                    const NfaStatesToTransitions &table,
                                    const NfaStateToCount        &counts,
                                    const NfaIdSet               &init,
                                    DfaObj                       &dfa) {
  NfaStatesToId map;
  return dfaFromNfaRecurse(multiChars, table, counts, init, map, nfa_, dfa);
}

///////////////////////////////////////////////////////////////////////////////

// returns a set of multi-chars that covers the input, with specific preferred
MultiCharSet basisMultiChars(const MultiCharSet &mcs) {
  // 1. arrange multi-chars by number of chars
  vector<MultiCharSet> sets;
  for (const MultiChar &mc : mcs) {
    CharIdx pop = mc.population();
    safeRef(sets, pop).insert(mc);
  }

  CharIdx mostBits = static_cast<CharIdx>(sets.size());

  // 2. calculate successive unions of multi-chars
  vector<MultiChar> unions;
  unions.resize(mostBits + 1);
  for (CharIdx ii = 1; ii <= mostBits; ++ii)
    for (CharIdx jj = 0; jj < ii; ++jj)
      for (const MultiChar &mc : sets[jj])
        unions[ii].unionWith(mc);

  // 3. subtract off corresponding unions
  MultiCharSet rv;
  for (CharIdx ii = 0; ii < mostBits; ++ii)
    for (const MultiChar &mc : sets[ii]) {
      MultiChar copy = mc;
      copy.subtract(unions[ii]);
      if (copy.population() > 0)
        rv.emplace(std::move(copy));
    }

  return rv;
}


// this is a performace-critical function
NfaStatesToTransitions makeTable(NfaId                    initial,
                                 const NfaObj            &nfa,
                                 const vector<MultiChar> &allMultiChars) {
  NfaStatesToTransitions table;

  const size_t allSize = allMultiChars.size();

  NfaIdSet initialStates;
  initialStates.set(initial);
  unordered_set<NfaIdSet> todoSet;
  todoSet.emplace(std::move(initialStates));

  while (!todoSet.empty()) {
    auto todoNode = todoSet.extract(todoSet.begin());
    NfaIdSet &todo = todoNode.value();

    std::pair<NfaIdSet, IdxToNfaIdSet> tableNode;
    tableNode.first = std::move(todo);
    auto [tableIt, novel] = table.emplace(std::move(tableNode));
    if (novel) {
      // iterate bit-set once, as it's more expensive to do
      for (NfaId id : tableIt->first)
        for (const NfaTransition &trans : nfa[id].transitions_)
          for (size_t idx = 0; idx < allSize; ++idx)
            if (allMultiChars[idx].hasIntersection(trans.multiChar_))
              tableIt->second[idx].set(trans.next_);
      for (const auto &[_, nis] : tableIt->second)
        todoSet.insert(nis);
    }
  }

  return table;
}


NfaStateToCount countAcceptingStates(const NfaStatesToTransitions &table,
                                     const NfaObj                 &nfa) {
  NfaStateToCount rv;
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
                        const NfaStateToCount        &counts,
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
    return dfaId; // FIXME can this happen? is it right?

  for (const auto &[ii, nis] : tableIter->second) {
    if (nis.population() > 0) {
      DfaId subId = dfaFromNfaRecurse(
          multiChars, table, counts, nis, map, nfa, dfa);
      for (CharIdx ch : multiChars[ii]) // FIXME why is mc[ii] valid???
        dfa[dfaId].trans_.set(ch, subId);
    }
  }

  if (nfa.hasAccept(stateSet))
    dfa[dfaId].result_ = getResult(stateSet, counts, nfa);

  return dfaId;
}

} // namespace zezax::red
