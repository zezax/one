// nfa to dfa converter header

#pragma once

#include <vector>

#include "Nfa.h"
#include "Dfa.h"

namespace zezax::red {

// FIXME class like minimizer

// This is the function you want
DfaObj convertNfaToDfa(const NfaObj &nfa);

// sometimes called translation or transition table...
typedef std::unordered_map<NfaIdSet,
                           std::vector<NfaIdSet>> NfaStatesToTransitions;

typedef std::unordered_map<NfaId, size_t> NfaStateToCount;

// this should compare by id
typedef std::unordered_map<NfaIdSet, DfaId> NfaStatesToId;

// utility functions
MultiCharSet basisMultiChars(const MultiCharSet &mcs);

NfaStatesToTransitions makeTable(NfaId                         initial,
                                 const NfaObj                 &nfa,
                                 const std::vector<MultiChar> &allMultiChars);

NfaStateToCount countAcceptingStates(const NfaStatesToTransitions &table,
                                     const NfaObj                 &nfa);

Result getResult(const NfaIdSet        &ss,
                 const NfaStateToCount &counts,
                 const NfaObj          &nfa);

DfaId dfaFromNfa(const std::vector<MultiChar> &multiChars,
                 const NfaStatesToTransitions &table,
                 const NfaStateToCount        &counts,
                 const NfaIdSet               &init,
                 NfaStatesToId                &map,
                 const NfaObj                 &nfa,
                 DfaObj                       &dfa);

} // namespace zezax::red
