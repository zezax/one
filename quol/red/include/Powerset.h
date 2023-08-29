/* Powerset.h - powerset NFA to DFA converter header

   PowersetConverter takes a non-deterministic finite automaton (NFA)
   as input and creates an equivalent determinitic automaton (DFA) as
   output.  The algorithm is Rabin-Scott powerset construction (1959)
   as described here:
   https://en.wikipedia.org/wiki/Powerset_construction
   https://www.cse.chalmers.se/~coquand/AUTOMATA/rs.pdf

   The basic idea is to simulate the NFA by keeping track of the set
   of states that it could be in, given each input so far.  The DFA is
   built by mapping sets of NFA states to single DFA states.  The
   resulting DFA is likely bloated and should be minimized.

   Worst-case performance is O(2^N) in both time and memory, where N
   is the number of NFA states.  Typical performance is better.

   This implementation assumes that the source NFA employs end-marks.
   Classic automata have only two results: reject (0) and accept (1).
   Endmarks are a way around this limitation, enableing multiple
   accepting result values.  End marks are extra states that are
   reached via an out-of-alphabet transition, the value of which
   indicates the result.  The resulting DFA will have the end marks
   properly interpreted and removed.

   If a Budget is supplied, it will be honored.  Also, a CompStats
   object can be given, if statistics are desired.

   Usage is like this:

   PowersetConverter power(nfa, budget, stats);
   DfaObj dfa = power.convert();

   PowersetConverter can throw RedExceptCompile for internal errors.
 */

#pragma once

#include <map>
#include <vector>

#include "Budget.h"
#include "Nfa.h"
#include "Dfa.h"

namespace zezax::red {

typedef SparseVec<NfaIdSet> IdxToNfaIdSet;

// sometimes called translation or transition table...
typedef std::unordered_map<NfaIdSet, IdxToNfaIdSet> NfaStatesToTransitions;

typedef std::unordered_map<NfaId, size_t> NfaIdToCount;

typedef std::unordered_map<NfaIdSet, DfaId> NfaStatesToId;


// converts nfa to dfa via rabin-scott
class PowersetConverter {
public:
  explicit PowersetConverter(const NfaObj &input,
                             Budget       *budget = nullptr,
                             CompStats    *stats  = nullptr)
    : nfa_(input), budget_(budget), stats_(stats) {}

  DfaObj convert();

private:
  DfaId dfaFromNfa(const std::vector<MultiChar> &multiChars,
                   const NfaStatesToTransitions &table,
                   const NfaIdToCount           &counts,
                   const NfaIdSet               &states,
                   DfaObj                       &dfa);

  const NfaObj &nfa_;
  Budget       *budget_;
  CompStats    *stats_;
};


// constituent functions, public for unit tests
MultiCharSet basisMultiChars(const MultiCharSet &mcs);

NfaStatesToTransitions makeTable(NfaId                         initial,
                                 const NfaObj                 &nfa,
                                 const std::vector<MultiChar> &allMultiChars);

NfaIdToCount countAcceptingStates(const NfaStatesToTransitions &table,
                                  const NfaObj                 &nfa);

DfaId dfaFromNfaRecurse(const std::vector<MultiChar> &multiChars,
                        const NfaStatesToTransitions &table,
                        const NfaIdToCount           &counts,
                        const NfaIdSet               &stateSet,
                        NfaStatesToId                &map,
                        const NfaObj                 &nfa,
                        DfaObj                       &dfa);

} // namespace zezax::red
