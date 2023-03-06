// general printing and debugging header

#pragma once

#include <limits>
#include <string>
#include <iostream>

#include "Types.h"
#include "Scanner.h"
#include "Nfa.h"
#include "Dfa.h"
#include "Powerset.h"
#include "Minimizer.h"
#include "Serializer.h"

namespace zezax::red {

std::string toString(const MultiChar &mc);
std::string toString(const MultiCharSet &mcs);
std::string toString(const NfaIdSet &nis);
std::string toString(const Token &t);
std::string toString(const NfaState &ns);
std::string toString(const NfaObj &nfa);
std::string toString(const NfaIdSet &nis, const NfaObj &nfa);
std::string toString(const NfaStatesToTransitions &tbl);
std::string toString(const NfaIdToCount &c);

std::string toString(const CharToStateMap &map);
std::string toString(const DfaIdSet &dis);
std::string toString(const DfaState &ds);
std::string toString(const DfaObj &dfa);
std::string toString(const DfaEdge &e);
std::string toString(const DfaEdgeSet &des);
std::string toString(const DfaEdgeToIds &rev);
std::string toString(const std::vector<DfaIdSet> &blocks);
std::string toString(const BlockRec &br);
std::string toString(const BlockRecSet &brs);
std::string toString(const std::vector<CharIdx> &vec); // equiv map

std::string toString(const FileHeader &hdr);
std::string toString(const char *buf, size_t len); // serialized dfa

std::string toString(const CompStats *s);

char toHexDigit(Byte x);


template <class T>
std::string toHexString(T x) {
  char buf[std::numeric_limits<T>::digits10 + 1];
  char *ptr = buf + sizeof(buf) - 1;
  *ptr = '\0';
  do {
    Byte nibble = x % 16u;
    x /= 16u;
    *--ptr = toHexDigit(nibble);
  } while (x != 0);
  return ptr;
}


// for readable gtest output
std::ostream &operator<<(std::ostream &os, const MultiChar &mc);
std::ostream &operator<<(std::ostream &os, const DfaIdSet &dis);

} // namespace zezax::red
