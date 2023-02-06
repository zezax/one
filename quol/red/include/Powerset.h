// powerset nfa to dfa converter header

#pragma once

#include <vector>

#include "Nfa.h"
#include "Dfa.h"

namespace zezax::red {

// sometimes called translation or transition table...
typedef std::unordered_map<NfaIdSet,
                           CappedVec<NfaIdSet>> NfaStatesToTransitions;

typedef std::unordered_map<NfaId, size_t> NfaStateToCount;

// this should compare by id
typedef std::unordered_map<NfaIdSet, DfaId> NfaStatesToId;


// converts nfa to dfa via rabin-scott
class PowersetConverter {
public:
  PowersetConverter(const NfaObj &input) : nfa_(input) {}

  DfaObj convert();

private:
  DfaId dfaFromNfa(const std::vector<MultiChar> &multiChars,
                   const NfaStatesToTransitions &table,
                   const NfaStateToCount        &counts,
                   const NfaIdSet               &init,
                   DfaObj                       &dfa);

  const NfaObj &nfa_;
};


// component functions, public for unit tests
MultiCharSet basisMultiChars(const MultiCharSet &mcs);

NfaStatesToTransitions makeTable(NfaId                         initial,
                                 const NfaObj                 &nfa,
                                 const std::vector<MultiChar> &allMultiChars);

NfaStateToCount countAcceptingStates(const NfaStatesToTransitions &table,
                                     const NfaObj                 &nfa);

DfaId dfaFromNfaRecurse(const std::vector<MultiChar> &multiChars,
                        const NfaStatesToTransitions &table,
                        const NfaStateToCount        &counts,
                        const NfaIdSet               &init,
                        NfaStatesToId                &map,
                        const NfaObj                 &nfa,
                        DfaObj                       &dfa);

} // namespace zezax::red
