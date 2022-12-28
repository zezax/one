// general printing and debugging header

#pragma once

#include <limits>
#include <string>
#include <iostream>

#include "Defs.h"
#include "BitSet.h"
#include "Scanner.h"
#include "Nfa.h"
#include "Dfa.h"
#include "NfaToDfa.h"
#include "DfaMinimizer.h"

namespace zezax::red {

// FIXME: optional line prefix arg
std::string toString(const MultiChar &mc);
std::string toString(const MultiCharSet &mcs);
std::string toString(const NfaIdSet &nis);
std::string toString(const StateIdSet &sis);
std::string toString(const std::vector<StateIdSet> &blocks);
std::string toString(const Token &t);
std::string toString(const NfaState &ns);
std::string toStringDeep(const NfaObj &nfa);
std::string toStringDeep(const NfaObj &nfa, NfaId id);
std::string toString(const NfaIdSet &nis, const NfaObj &nfa);
std::string toString(const NfaStatesToTransitions &tbl);
std::string toString(const NfaStateToCount &c);

std::string toString(const CharToStateMap &map);
std::string toString(const DfaState &ds);
std::string toString(const DfaObj &dfa);
std::string toString(const DfaEdge &e);
std::string toString(const DfaEdgeSet &des);
std::string toString(const DfaEdgeToIds &rev);
std::string toString(const BlockRec &br);
std::string toString(const BlockRecSet &brs);

std::string toString(const std::vector<CharIdx> &vec);

char toHexDigit(Byte x);

template <class T>
std::string toHexString(T x) {
  char buf[std::numeric_limits<T>::digits10 + 1];
  char *ptr = buf + sizeof(buf) - 1;
  *ptr = '\0';
  do {
    Byte nibble = x & 0xf;
    *--ptr = toHexDigit(nibble);
    x >>= 4;
  } while (x != 0);
  return ptr;
}

// for readable gtest output
std::ostream &operator<<(std::ostream &os, const MultiChar &mc);
std::ostream &operator<<(std::ostream &os, const StateIdSet &sis);

} // namespace zezax::red
