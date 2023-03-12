// general printing and debugging implementation
// in theory, none of this code should end up linked into a final binary

#include "Debug.h"

#include <algorithm>
#include <limits>
#include <set>
#include <ratio>
#include <vector>

#include "Proxy.h"

namespace zezax::red {

namespace chrono = std::chrono;

using chrono::duration_cast;
using chrono::nanoseconds;
using chrono::steady_clock;
using std::numeric_limits;
using std::ostream;
using std::string;
using std::to_string;
using std::vector;

namespace {

string visibleChar(CharIdx ch) {
  string rv;
  if ((ch >= ' ') && (ch <= '~')) // simple printable
    rv += static_cast<char>(ch);
  else if (ch <= 255) {
    Byte c8 = static_cast<Byte>(ch);
    if (c8 < ' ') {               // control
      rv += '^';
      rv += '@' + static_cast<char>(c8);
    }
    else {                        // high ascii in hex
      rv += '$';
      rv += toHexDigit(c8 / 16);
      rv += toHexDigit(c8 % 16);
    }
  }
  else {                          // render end-marks in brackets
    rv += '[';
    rv += to_string(ch - gAlphabetSize);
    rv += ']';
  }
  return rv;
}


string timeDiff(steady_clock::time_point aa, steady_clock::time_point bb) {
  auto xx = duration_cast<nanoseconds>(aa.time_since_epoch()).count();
  auto yy = duration_cast<nanoseconds>(bb.time_since_epoch()).count();
  double dd = static_cast<double>(yy - xx);
  dd /= std::nano::den;
  return to_string(dd);
}


template <Format fmt>
void appendSerializedState(string     &buf,
                           const char *ptr,
                           size_t      off,
                           size_t      maxChar) {
  DfaProxy<fmt> proxy;
  proxy.init(ptr, 0);
  Result result = proxy.result();
  bool deadEnd = proxy.deadEnd();
  buf += '$' + toHexString(off) + " -> " + to_string(result) + '\n';
  if (deadEnd)
    buf += "  DeadEnd\n";
  for (size_t ii = 0; ii <= maxChar; ++ii) {
    buf += "  " + to_string(ii) + " -> $" +
      toHexString(proxy.trans(ii)) + '\n';
  }
}


// BitSet
template <class Index, class Tag, class Word>
void toStringAppend(string &out, const BitSet<Index, Tag, Word> &bs) {
  constexpr Index nval = numeric_limits<Index>::max() - 1; // avoid opt bug
  Index start = 0;
  Index prev = nval;
  bool first = true;
  for (Index cur : bs) {
    if (cur != (prev + 1)) {
      if (prev != nval) {
        if (first)
          first = false;
        else
          out += ',';
        out += to_string(start);
        if (prev > start) {
          out += '-';
          out += to_string(prev);
        }
      }
      start = cur;
    }
    prev = cur;
  }
  if (prev != nval) {
    if (!first)
      out += ',';
    out += to_string(start);
    if (prev > start) {
      out += '-';
      out += to_string(prev);
    }
  }
}


// NfaState
void toStringAppend(string &out, const NfaState &ns) {
  out += "NfaState -> " + to_string(ns.result_) + '\n';
  for (const NfaTransition &tr : ns.transitions_)
    out += "  " + to_string(tr.next_) + " <- " +
      toString(tr.multiChar_) + '\n';
}


// DfaState
void toStringAppend(string &out, const DfaState &ds) {
  out += "DfaState -> " + to_string(ds.result_) + '\n';
  if (ds.deadEnd_)
    out += "  DeadEnd\n";
  out += toString(ds.trans_);
}


// equivalence map
template <class T>
void toStringAppendEquivMap(string &out, const T *ptr, size_t len) {
  out += "equiv[\n";
  constexpr size_t nval = numeric_limits<size_t>::max();
  size_t start = 0;
  size_t prev = nval;
  size_t idx;
  for (idx = 0; idx < len; ++idx) {
    size_t ch = ptr[idx];
    if (ch != prev) {
      if (prev != nval) {
        out += "  " + to_string(prev) + " <- " +
          visibleChar(static_cast<CharIdx>(start));
        if ((idx - 1) > start) {
          out += '-';
          out += visibleChar(static_cast<CharIdx>(idx - 1));
        }
        out += '\n';
      }
      start = idx;
    }
    prev = ch;
  }
  if (prev != nval) {
    --idx;
    out += "  " + to_string(prev) + " <- " +
      visibleChar(static_cast<CharIdx>(start));
    if (idx > start) {
      out += '-';
      out += visibleChar(static_cast<CharIdx>(idx));
    }
    out += '\n';
  }
  out += "]\n";
}


// equivalence map
void toStringAppend(string &out, const vector<CharIdx> &vec) {
  toStringAppendEquivMap(out, vec.data(), vec.size());
}


// FileHeader
void toStringAppend(string &out, const FileHeader &hdr) {
  out += visibleChar(hdr.magic_[0]);
  out += visibleChar(hdr.magic_[1]);
  out += visibleChar(hdr.magic_[2]);
  out += visibleChar(hdr.magic_[3]);
  out += '/' + to_string(static_cast<unsigned>(hdr.majVer_)) +
    '.' + to_string(static_cast<unsigned>(hdr.minVer_)) + '\n';
  out += "csum=0x" + toHexString(hdr.checksum_) +
    " fmt=" + to_string(static_cast<unsigned>(hdr.format_)) +
    " maxChar=" + to_string(static_cast<unsigned>(hdr.maxChar_)) +
    " leaderLen=" + to_string(static_cast<unsigned>(hdr.leaderLen_)) + '\n';
  out += "states=" + to_string(hdr.stateCnt_) +
    " init=$" + toHexString(hdr.initialOff_) + '\n';
}

} // anonymous

///////////////////////////////////////////////////////////////////////////////
//
// NFA STUFF
//
///////////////////////////////////////////////////////////////////////////////

// MultiChar
string toString(const MultiChar &mc) {
  string rv;
  CharIdx start = 0;
  CharIdx prev = ~0U;
  for (CharIdx cur : mc) {
    if (cur != (prev + 1)) {
      if (prev != ~0U) {
        rv += visibleChar(start);
        if (prev > start) {
          if (prev > (start + 1)) // prefer ab to a-b
            rv += '-';
          rv += visibleChar(prev);
        }
      }
      start = cur;
    }
    prev = cur;
  }
  if (prev != ~0U) {
    rv += visibleChar(start);
    if (prev > start) {
      if (prev > (start + 1))
        rv += '-';
      rv += visibleChar(prev);
    }
  }
  return rv;
}


// MultiCharSet
string toString(const MultiCharSet &mcs) {
  vector<MultiChar> vec;
  for (const MultiChar &mc : mcs)
    vec.push_back(mc);
  std::sort(vec.begin(), vec.end());
  string rv;
  for (const MultiChar &mc : vec)
    rv += toString(mc) + '\n';
  return rv;
}


// NfaIdSet as numbers
string toString(const NfaIdSet &nis) {
  string rv;
  toStringAppend(rv, nis);
  return rv;
}


// Token
string toString(const Token &t) {
  switch (t.type_) {
  case tError:   return "error";
  case tEnd:     return "end";
  case tFlags:   return "flags " + toHexString(t.flags_);
  case tChars:   return "chars " + toString(t.multiChar_);
  case tClosure: return "close " + to_string(t.min_) + ':' + to_string(t.max_);
  case tUnion:   return "union";
  case tLeft:    return "left";
  case tRight:   return "right";
  }
  throw RedExceptInternal("bad token type");
}


// NfaState
string toString(const NfaState &ns) {
  string rv;
  toStringAppend(rv, ns);
  return rv;
}


// NfaObj state dump
string toString(const NfaObj &nfa) {
  vector<NfaId> vec;
  for (NfaConstIter it = nfa.citer(nfa.getInitial()); it; ++it)
    vec.push_back(it.id());
  std::sort(vec.begin(), vec.end());

  string rv;
  for (NfaId id : vec) {
    rv += to_string(id) + ' ';
    toStringAppend(rv, nfa[id]);
    rv += '\n';
  }
  return rv;
}


// NfaIdSet state dump
string toString(const NfaIdSet &nis, const NfaObj &nfa) {
  string rv = "set{\n";
  for (NfaId id : nis) {
    rv += to_string(id) + ' ';
    toStringAppend(rv, nfa[id]);
    rv += '\n';
  }
  return rv + "}\n";
}


// NfaStatesToTransitions map/table
string toString(const NfaStatesToTransitions &tbl) {
  vector<NfaIdSet> vec;
  for (const auto &[key, _] : tbl)
    vec.emplace_back(key);
  std::sort(vec.begin(), vec.end());

  string rv = "map(\n";
  for (const NfaIdSet &key : vec) {
    const IdxToNfaIdSet &val = tbl.at(key);
    rv += "key=" + toString(key) + " val=[";
    bool first = true;
    for (const auto &[_, nis] : val) {
      if (first)
        first = false;
      else
        rv += ';';
      rv += toString(nis);
    }
    rv += "]\n";
  }
  return rv + ")\n";
}


// NfaIdToCount
string toString(const NfaIdToCount &c) {
  vector<std::pair<NfaId, size_t>> vec;
  for (auto [key, val] : c)
    vec.emplace_back(key, val);
  std::sort(vec.begin(), vec.end());

  string rv = "counts(\n";
  for (auto [key, val] : vec)
    rv += "key=" + to_string(key) + " val=" + to_string(val) + '\n';
  return rv + ")\n";
}

///////////////////////////////////////////////////////////////////////////////
//
// DFA STUFF
//
///////////////////////////////////////////////////////////////////////////////

// CharToStateMap
string toString(const CharToStateMap &map) {
  vector<std::pair<CharIdx, DfaId>> vec;
  for (auto [ch, id] : map.getMap())
    vec.emplace_back(ch, id);
  std::sort(vec.begin(), vec.end());

  string rv;
  CharIdx start = 0;
  CharIdx prevCh = ~0U;
  DfaId prevId = -1;
  for (auto [ch, id] : vec) {
    if ((id != prevId) || (ch != (prevCh + 1))) {
      if (prevCh != ~0U) {
        rv += "  " + to_string(prevId) + " <- " + visibleChar(start);
        if (prevCh > start) {
          if (prevCh > (start + 1)) // ab is better than a-b
            rv += '-';
          rv += visibleChar(prevCh);
        }
        rv += '\n';
      }
      start = ch;
    }
    prevCh = ch;
    prevId = id;
  }
  if (prevCh != ~0U) {
    rv += "  " + to_string(prevId) + " <- " + visibleChar(start);
    if (prevCh > start) {
      if (prevCh > (start + 1))
        rv += '-';
      rv += visibleChar(prevCh);
    }
    rv += '\n';
  }
  return rv;
}


// DfaIdSet as numbers
string toString(const DfaIdSet &dis) {
  string rv;
  toStringAppend(rv, dis);
  return rv;
}


// DfaState
string toString(const DfaState &ds) {
  string rv;
  toStringAppend(rv, ds);
  return rv;
}


// DfaObj state dump
string toString(const DfaObj &dfa) {
  string rv = "Dfa init=" + to_string(gDfaInitialId) +
    " err=" + to_string(gDfaErrorId) + " [\n";
  DfaId ii = 0;
  for (const DfaState &ds : dfa.getStates()) {
    rv += to_string(ii) + ' ';
    toStringAppend(rv, ds);
    ++ii;
  }
  return rv + "]\n";
}


// DfaEdge
string toString(const DfaEdge &e) {
  return "edge(" + to_string(e.id_) + ' ' + visibleChar(e.char_) + ')';
}


// DfaEdgeSet
string toString(const DfaEdgeSet &des) {
  string rv = "edges{\n";
  for (const DfaEdge &e : des)
    rv += "  " + toString(e) + '\n';
  return rv + "}\n";
}


// DfaEdgeToIds (reverse map)
string toString(const DfaEdgeToIds &rev) {
  string rv = "rev{\n";
  for (const auto &[e, dis] : rev)
    rv += "  " + toString(e) + " -> " + toString(dis) + '\n';
  return rv + "}\n";
}


// blocks
string toString(const vector<DfaIdSet> &blocks) {
  string rv = "blocks[\n";
  int ii = 0;
  for (const DfaIdSet &dis : blocks) {
    rv += "  " + to_string(ii) + ": ";
    toStringAppend(rv, dis);
    rv += '\n';
    ++ii;
  }
  return rv + "]\n";;
}


// BlockRec
string toString(const BlockRec &br) {
  return "brec(" + to_string(br.block_) + ' ' + visibleChar(br.char_) + ')';
}


// BlockRecSet
string toString(const BlockRecSet &brs) {
  string rv = "brecs{\n";
  for (const BlockRec &br : brs)
    rv += "  " + toString(br) + '\n';
  return rv + "}\n";
}


// equivalence map
string toString(const vector<CharIdx> &vec) {
  string rv;
  toStringAppend(rv, vec);
  return rv;
}

///////////////////////////////////////////////////////////////////////////////
//
// SERIALIZED FORM
//
///////////////////////////////////////////////////////////////////////////////

// FileHeader
string toString(const FileHeader &hdr) {
  string rv;
  toStringAppend(rv, hdr);
  return rv;
}


// serialized dfa
string toString(const char *buf, size_t len) {
  string rv;
  const char *msg = checkHeader(buf, len);
  if (msg) {
    rv = msg;
    return rv;
  }

  const FileHeader *hdr = reinterpret_cast<const FileHeader *>(buf);
  if ((hdr->majVer_ != 1) && (hdr->minVer_ != 0)) {
    rv = "REDA version not 1.0";
    return rv;
  }

  toStringAppend(rv, *hdr);
  toStringAppendEquivMap(rv, hdr->equivMap_, sizeof(hdr->equivMap_));

  unsigned leaderLen = hdr->leaderLen_;
  if (leaderLen > 0) {
    rv += "leader=";
    for (unsigned uu = 0; uu < leaderLen; ++uu) {
      rv += to_string(static_cast<unsigned>(hdr->bytes_[uu]));
      rv += ',';
    }
    rv.pop_back();
    rv += '\n';
  }
  leaderLen = (leaderLen + 7) & ~7U; // round up to nearest 8

  Format fmt = static_cast<Format>(hdr->format_);
  switch (fmt) {
  case fmtDirect1:
  case fmtDirect2:
  case fmtDirect4:
    break;
  default:
    rv = "format not recognized";
    return rv;
  }

  CharIdx maxChar = hdr->maxChar_;
  const char *end = buf + len;
  const char *base = buf + sizeof(FileHeader) + leaderLen;

  size_t inc;
  switch (fmt) {
  case fmtDirect1:
    inc = DfaProxy<fmtDirect1>::stateSize(maxChar);
    break;
  case fmtDirect2:
    inc = DfaProxy<fmtDirect2>::stateSize(maxChar);
    break;
  case fmtDirect4:
    inc = DfaProxy<fmtDirect4>::stateSize(maxChar);
    break;
  default:
    throw RedExceptInternal("corrupted format");
  }

  for (const char *ptr = base; ptr < end; ptr += inc) {
    size_t off = static_cast<size_t>(ptr - base);
    switch (fmt) {
    case fmtDirect1:
      appendSerializedState<fmtDirect1>(rv, ptr, off, maxChar);
      break;
    case fmtDirect2:
      appendSerializedState<fmtDirect2>(rv, ptr, off, maxChar);
      break;
    case fmtDirect4:
      appendSerializedState<fmtDirect4>(rv, ptr, off, maxChar);
      break;
    default:
      break;
    }
  }

  return rv + "END\n";
}

///////////////////////////////////////////////////////////////////////////////
//
// MISC
//
///////////////////////////////////////////////////////////////////////////////

string toString(const CompStats *s) {
  string rv = "CompStats:\n";
  if (!s)
    return rv;
  rv += "totalTime   " + timeDiff(s->preNfa_, s->postSerialize_) + '\n';
  rv += "parseNfa    " + timeDiff(s->preNfa_, s->postNfa_) + '\n';
  rv += "buildDfa    " + timeDiff(s->preDfa_, s->postDfa_) + '\n';
  rv += "  basis     " + timeDiff(s->preDfa_, s->postBasisChars_) + '\n';
  rv += "  table     " + timeDiff(s->postBasisChars_, s->postMakeTable_) + '\n';
  rv += "  build     " + timeDiff(s->postMakeTable_, s->postDfa_) + '\n';
  rv += "minimizeDfa " + timeDiff(s->preMinimize_, s->postMinimize_) + '\n';
  rv += "  equivMap  " + timeDiff(s->preMinimize_, s->postEquivMap_) + '\n';
  rv += "  invert    " + timeDiff(s->postEquivMap_, s->postInvert_) + '\n';
  rv += "  partition " + timeDiff(s->postInvert_, s->postPartition_) + '\n';
  rv += "  makeList  " + timeDiff(s->postPartition_, s->postMakeList_) + '\n';
  rv += "  iterate   " + timeDiff(s->postMakeList_, s->postMinimize_) + '\n';
  rv += "serialize   " + timeDiff(s->preSerialize_, s->postSerialize_) + '\n';
  rv += "tokens               " + to_string(s->numTokens_) + '\n';
  rv += "patterns             " + to_string(s->numPatterns_) + '\n';
  rv += "origNfaStates        " + to_string(s->origNfaStates_) + '\n';
  rv += "usefulNfaStates      " + to_string(s->usefulNfaStates_) + '\n';
  rv += "origDfaStates        " + to_string(s->origDfaStates_) + '\n';
  rv += "minimizedDfaStates   " + to_string(s->minimizedDfaStates_) + '\n';
  rv += "serializedBytes      " + to_string(s->serializedBytes_) + '\n';
  rv += "distinguishedSymbols " + to_string(s->numDistinguishedSymbols_) + '\n';
  rv += "transitionTableRows  " + to_string(s->transitionTableRows_) + '\n';
  rv += "powersetMemUsed      " + to_string(s->powersetMemUsed_) + '\n';
  return rv;
}


char toHexDigit(Byte x) {
  if (x <= 9)
    return static_cast<char>('0' + x);
  if (x <= 15)
    return static_cast<char>('a' - 10 + x);
  throw RedExceptInternal("hex digit out of range");
}

///////////////////////////////////////////////////////////////////////////////

ostream &operator<<(ostream &os, const MultiChar &mc) {
  os << toString(mc);
  return os;
}

ostream &operator<<(ostream &os, const DfaIdSet &dis) {
  os << toString(dis);
  return os;
}

} // namespace zezax::red
