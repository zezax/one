// nfa to dfa converter implementation

#include "Except.h"
#include "Util.h"
#include "NfaToDfa.h"

namespace zezax::red {

using std::unordered_set;
using std::vector;

///////////////////////////////////////////////////////////////////////////////

DfaObj convertNfaToDfa(const NfaObj &nfa) {
  NfaState *initial = nfa.getNfaInitial();
  NfaStateSet states = allStates(initial);

  vector<MultiChar> multiChars;
  {
    MultiCharSet allMcs = allMultiChars(initial);
    MultiCharSet basisMcs = basisMultiChars(allMcs);
    allMcs.clear(); // save memory
    for (auto it = basisMcs.begin(); it != basisMcs.end(); ++it)
      multiChars.emplace_back(std::move(*it));
  }

  NfaStatesToTransitions table = makeTable(initial, multiChars);
  NfaStateToCount counts = countAcceptingStates(table);

  DfaObj dfa;
  StateId id = dfa.newState();
  if (id != gDfaErrorId)
    throw RedExcept("dfa error state must be zero");

  NfaStateSet initialStates;
  initialStates.insert(initial);
  NfaStatesToId nfaToDfa;
  auto it = table.find(initialStates);
  if (it == table.end())
    throw RedExcept("cannot find initial nfa states");
  id = dfaFromNfa(multiChars, table, counts,
                  initialStates, nfaToDfa, dfa);
  if (id != gDfaInitialId)
    throw RedExcept("dfa initial state must be one");
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
      // FIXME: if (copy.population() > 0)
      rv.emplace(std::move(copy));
    }

  return rv;
}


NfaStatesToTransitions makeTable(NfaState *initial,
                                 vector<MultiChar> &allMultiChars) {
  NfaStatesToTransitions table;

  NfaStateSet initialStates;
  initialStates.insert(initial);
  unordered_set<NfaStateSet> todoSet;
  todoSet.emplace(std::move(initialStates));

  while (!todoSet.empty()) {
    auto todoNode = todoSet.extract(todoSet.begin());
    NfaStateSet &todo = todoNode.value();

    std::pair<NfaStateSet, vector<NfaStateSet>> tableNode;
    tableNode.first = todo;
    auto [tableIt, novel] = table.emplace(std::move(tableNode));
    if (novel) {
      size_t idx = 0;
      for (MultiChar &mc : allMultiChars) {
        for (NfaState *state : todo)
          for (NfaTransition &trans : state->transitions_)
            if (mc.hasIntersection(trans.multiChar_))
              safeRef(tableIt->second, idx).insert(trans.next_);
        ++idx;
      }
      for (NfaStateSet &ss : tableIt->second)
        todoSet.insert(ss);
    }
  }

  return table;
}


NfaStateToCount countAcceptingStates(const NfaStatesToTransitions &table) {
  NfaStateToCount rv;
  for (auto tableIt = table.begin(); tableIt != table.end(); ++tableIt)
    for (const NfaState *ns : tableIt->first)
      if (accepts(ns)) {
        auto [it, _] = rv.emplace(ns, 0);
        ++it->second;
      }
  return rv;
}


Result getResult(const NfaStateSet &ss, const NfaStateToCount &counts) {
  Result rv = -1;
  size_t min = ~0UL;

  for (const NfaState *state : ss) {
    auto it = counts.find(state);
    if (it != counts.end()) {
      size_t num = it->second;
      if (num < min) {
        min = num;
        rv = state->result_;
      }
    }
  }

  return rv;
}


StateId dfaFromNfa(const vector<MultiChar> &multiChars,
                   const NfaStatesToTransitions &table,
                   const NfaStateToCount &counts,
                   const NfaStateSet &states,
                   NfaStatesToId &map,
                   DfaObj &dfa) {
  std::pair<NfaStateSet, StateId> mapNode;
  mapNode.first = states;
  auto [mapIter, novel] = map.emplace(std::move(mapNode));
  if (!novel)
    return mapIter->second;

  StateId dfaId = dfa.newState();
  mapIter->second = dfaId;

  auto tableIter = table.find(states);
  if (tableIter == table.end())
    return dfaId; // FIXME ???

  StateId ii = 0;
  for (const NfaStateSet &ss : tableIter->second) {
    if (!ss.empty()) {
      StateId subId = dfaFromNfa(multiChars, table, counts, ss, map, dfa);
      for (CharIdx ch : multiChars[ii]) // FIXME why is mc[ii] valid???
        dfa[dfaId].trans_.set(ch, subId);
    }
    ++ii;
  }

  if (hasAccept(states))
    dfa[dfaId].result_ = getResult(states, counts);

  return dfaId;
}

} // namespace zezax::red
