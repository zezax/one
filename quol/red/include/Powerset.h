// powerset nfa to dfa converter header

#pragma once

#include <map>
#include <vector>

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
  explicit PowersetConverter(const NfaObj &input, CompStats *stats = nullptr)
    : nfa_(input), stats_(stats) {}

  DfaObj convert();

private:
  DfaId dfaFromNfa(const std::vector<MultiChar> &multiChars,
                   const NfaStatesToTransitions &table,
                   const NfaIdToCount           &counts,
                   const NfaIdSet               &states,
                   DfaObj                       &dfa);

  const NfaObj &nfa_;
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
