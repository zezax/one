// nfa to dfa converter header

#pragma once

#include <vector>

#include "Nfa.h"
#include "Dfa.h"

namespace zezax::red {

// This is the function you want
DfaObj convertNfaToDfa(const NfaObj &nfa);

// sometimes called translation or transition table...
typedef std::unordered_map<NfaStateSet,
                           std::vector<NfaStateSet>> NfaStatesToTransitions;

typedef std::unordered_map<const NfaState *, size_t> NfaStateToCount;

//FIXME: make sure this compares by id
typedef std::unordered_map<NfaStateSet, StateId> NfaStatesToId;

// utility functions
MultiCharSet basisMultiChars(const MultiCharSet &mcs);

NfaStatesToTransitions makeTable(NfaState               *initial,
                                 std::vector<MultiChar> &allMultiChars);

NfaStateToCount countAcceptingStates(const NfaStatesToTransitions &table);

Result getResult(const NfaStateSet &ss, const NfaStateToCount &counts);

StateId dfaFromNfa(const std::vector<MultiChar> &multiChars,
                   const NfaStatesToTransitions &table,
                   const NfaStateToCount        &counts,
                   const NfaStateSet            &init,
                   NfaStatesToId                &map,
                   DfaObj                       &dfa);

} // namespace zezax::red
