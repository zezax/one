// nfa to dfa converter implementation

#include <limits>

#include "Except.h"
#include "Util.h"
#include "NfaToDfa.h"

namespace zezax::red {

using std::numeric_limits;
using std::unordered_set;
using std::vector;

///////////////////////////////////////////////////////////////////////////////

DfaObj convertNfaToDfa(const NfaObj &nfa) {
  NfaId initial = nfa.getNfaInitial();

  vector<MultiChar> multiChars;
  {
    MultiCharSet allMcs = nfa.allMultiChars(initial);
    MultiCharSet basisMcs = basisMultiChars(allMcs);
    allMcs.clear(); // save memory
    for (auto it = basisMcs.begin(); it != basisMcs.end(); ++it)
      multiChars.emplace_back(std::move(*it));
  }

  NfaStatesToTransitions table = makeTable(initial, nfa, multiChars);
  NfaStateToCount counts = countAcceptingStates(table, nfa);

  DfaObj dfa;
  StateId id = dfa.newState();
  if (id != gDfaErrorId)
    throw RedExceptCompile("dfa error state must be zero");

  NfaIdSet states;
  states.set(initial);
  NfaStatesToId nfaToDfa;
  auto it = table.find(states);
  if (it == table.end())
    throw RedExceptCompile("cannot find initial nfa states");
  id = dfaFromNfa(multiChars, table, counts, states, nfaToDfa, nfa, dfa);
  if (id != gDfaInitialId)
    throw RedExceptCompile("dfa initial state must be one");
  dfa.chopEndMarks(); // end marks have done their job
  return dfa;
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

  NfaIdSet initialStates;
  initialStates.set(initial);
  unordered_set<NfaIdSet> todoSet;
  todoSet.emplace(std::move(initialStates));

  while (!todoSet.empty()) {
    auto todoNode = todoSet.extract(todoSet.begin());
    NfaIdSet &todo = todoNode.value();

    std::pair<NfaIdSet, vector<NfaIdSet>> tableNode;
    tableNode.first = std::move(todo);
    auto [tableIt, novel] = table.emplace(std::move(tableNode));
    if (novel) {
      size_t n = allMultiChars.size();
      for (size_t idx = 0; idx < n; ++idx) {
        const MultiChar &mc = allMultiChars[idx];
        for (NfaId id : tableIt->first)
          for (const NfaTransition &trans : nfa[id].transitions_)
            if (mc.hasIntersection(trans.multiChar_))
              safeRef(tableIt->second, idx).set(trans.next_);
      }
      for (NfaIdSet &nis : tableIt->second)
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


StateId dfaFromNfa(const vector<MultiChar>      &multiChars,
                   const NfaStatesToTransitions &table,
                   const NfaStateToCount        &counts,
                   const NfaIdSet               &stateSet,
                   NfaStatesToId                &map,
                   const NfaObj                 &nfa,
                   DfaObj                       &dfa) {
  std::pair<NfaIdSet, StateId> mapNode;
  mapNode.first = stateSet;
  auto [mapIter, novel] = map.emplace(std::move(mapNode));
  if (!novel)
    return mapIter->second;

  StateId dfaId = dfa.newState();
  mapIter->second = dfaId;

  auto tableIter = table.find(stateSet);
  if (tableIter == table.end())
    return dfaId; // FIXME can this happen? is it right?

  StateId ii = 0;
  for (const NfaIdSet &nis : tableIter->second) {
    if (nis.population() > 0) {
      StateId subId = dfaFromNfa(multiChars, table, counts, nis, map, nfa, dfa);
      for (CharIdx ch : multiChars[ii]) // FIXME why is mc[ii] valid???
        dfa[dfaId].trans_.set(ch, subId);
    }
    ++ii;
  }

  if (nfa.hasAccept(stateSet))
    dfa[dfaId].result_ = getResult(stateSet, counts, nfa);

  return dfaId;
}

} // namespace zezax::red
