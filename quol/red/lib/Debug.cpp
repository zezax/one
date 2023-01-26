// general printing and debugging implementation

#include "Debug.h"

#include <algorithm>
#include <set>
#include <vector>

#include "Proxy.h"

namespace zezax::red {

using std::ostream;
using std::string;
using std::to_string;
using std::vector;

namespace {

string visibleChar(CharIdx ch) {
  string rv;
  if ((ch >= ' ') && (ch <= '~'))
    rv += static_cast<char>(ch);
  else if (ch <= 255) {
    Byte c8 = static_cast<Byte>(ch);
    if (c8 < ' ') {
      rv += '^';
      rv += '@' + static_cast<char>(c8);
    }
    else {
      rv += '$';
      rv += toHexDigit(c8 >> 4);
      rv += toHexDigit(c8 & 0x0f);
    }
  }
  else {
    rv += '[';
    rv += to_string(ch - gAlphabetSize);
    rv += ']';
  }
  return rv;
}


void toStringAppend(string &out, const NfaIdSet &nis) {
  bool first = true;
  for (NfaIdSetIter it = nis.begin(); it != nis.end(); ++it) {
    if (first)
      first = false;
    else
      out += ',';
    out += to_string(*it);
  }
}


void toStringAppend(string &out, const StateIdSet &sis) {
  bool first = true;
  for (StateIdSetIter it = sis.begin(); it != sis.end(); ++it) {
    if (first)
      first = false;
    else
      out += ',';
    out += to_string(*it);
  }
}


void toStringAppend(string &out, const NfaState &ns) {
  out += "NfaState -> " + to_string(ns.result_) + '\n';
  for (const NfaTransition &tr : ns.transitions_)
    out += "  " + to_string(tr.next_) + " <- " +
      toString(tr.multiChar_) + '\n';
}


void toStringAppend(string &out, const DfaState &ds) {
  out += "DfaState -> " + to_string(ds.result_) + '\n';
  if (ds.deadEnd_)
    out += "  DeadEnd\n";
  out += toString(ds.trans_);
}


template <class T>
void toStringAppendEquivMap(string &out, const T *ptr, size_t len) {
  out += "equiv[\n";
  size_t nval = 0;
  --nval;  // underflow to largest 2's complement
  size_t start = 0;
  size_t prev = nval;
  size_t idx;
  for (idx = 0; idx < len; ++idx) {
    size_t ch = ptr[idx];
    if (ch != prev) {
      if (prev != nval) {
        out += "  " + to_string(prev) + " <- " + to_string(start);
        if ((idx - 1) > start) {
          out += '-';
          out += to_string(idx - 1);
        }
        out += '\n';
      }
      start = idx;
    }
    prev = ch;
  }
  if (prev != nval) {
    --idx;
    out += "  " + to_string(prev) + " <- " + to_string(start);
    if (idx > start) {
      out += '-';
      out += to_string(idx);
    }
    out += '\n';
  }
  out += "]\n";
}


void toStringAppend(string &out, const vector<CharIdx> &vec) {
  toStringAppendEquivMap(out, vec.data(), vec.size());
}


void toStringAppend(string &out, const FileHeader &hdr) {
  out += visibleChar(hdr.magic_[0]);
  out += visibleChar(hdr.magic_[1]);
  out += visibleChar(hdr.magic_[2]);
  out += visibleChar(hdr.magic_[3]);
  out += '/' + to_string(static_cast<unsigned>(hdr.majVer_)) +
    '.' + to_string(static_cast<unsigned>(hdr.minVer_)) + '\n';
  out += "csum=0x" + toHexString(hdr.checksum_) +
    " fmt=" + to_string(static_cast<unsigned>(hdr.format_)) +
    " maxChar=" + to_string(static_cast<unsigned>(hdr.maxChar_)) + '\n';
  out += "states=" + to_string(hdr.stateCnt_) +
    " init=$" + toHexString(hdr.initialOff_) + '\n';
}

} // anonymous

string toString(const MultiChar &mc) {
  string rv;
  CharIdx start = 0;
  CharIdx prev = ~0U;
  for (MultiCharIter it = mc.begin(); it != mc.end(); ++it) {
    CharIdx cur = *it;
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


string toString(const MultiCharSet &mcs) {
  string rv;
  for (const MultiChar &mc : mcs)
    rv += toString(mc) + '\n';
  return rv;
}


string toString(const NfaIdSet &nis) {
  string rv;
  toStringAppend(rv, nis);
  return rv;
}


string toString(const StateIdSet &sis) {
  string rv;
  toStringAppend(rv, sis);
  return rv;
}


string toString(const vector<StateIdSet> &blocks) {
  string rv = "blocks[\n";
  int ii = 0;
  for (const StateIdSet &sis : blocks) {
    rv += "  " + to_string(ii) + ": ";
    toStringAppend(rv, sis);
    rv += '\n';
    ++ii;
  }
  return rv + "]\n";;
}


string toString(const Token &t) {
  switch (t.type_) {
  case tError:   return "error";
  case tEnd:     return "end";
  case tFlags:   return "flags " + toHexString(t.flags_);
  case tChars:   return "chars " + toString(t.multiChar_);
  case tClosure: return "close " + to_string(t.min_) + ':' + to_string(t.max_);
  case tBar:     return "bar";
  case tLeft:    return "left";
  case tRight:   return "right";
  }
  throw RedExceptInternal("bad token type");
}


string toString(const NfaObj &nfa) {
  NfaIdSet all = nfa.allStates(nfa.getNfaInitial());
  vector<NfaId> vec;
  for (NfaId id : all)
    vec.push_back(id);
  std::sort(vec.begin(), vec.end());

  string rv;
  for (NfaId id : vec) {
    rv += to_string(id) + ' ';
    toStringAppend(rv, nfa[id]);
    rv += '\n';
  }
  return rv;
}


string toString(const NfaIdSet &nis, const NfaObj &nfa) {
  string rv = "set{\n";
  for (NfaId id : nis) {
    rv += to_string(id) + ' ';
    toStringAppend(rv, nfa[id]);
    rv += '\n';
  }
  return rv + "}\n";
}


string toString(const NfaStatesToTransitions &tbl) {
  string rv = "map(\n";
  for (auto &[key, val] : tbl) {
    rv += "key:" + toString(key) + "val:[";
    for (const NfaIdSet &nis : val)
      rv += toString(nis);
    rv += "]\n";
  }
  return rv + ")\n";
}


string toString(const NfaStateToCount &c) {
  string rv = "counts(\n";
  for (auto [key, val] : c)
    rv += "key:" + to_string(key) + "val:" + to_string(val);
  return rv + ")\n";
}

///////////////////////////////////////////////////////////////////////////////

string toString(const CharToStateMap &map) {
  vector<std::pair<CharIdx, StateId>> vec;
  for (auto [ch, id] : map.getMap())
    vec.emplace_back(ch, id);
  std::sort(vec.begin(), vec.end());

  string rv;
  CharIdx start = 0;
  CharIdx prevCh = ~0U;
  StateId prevId = -1;
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


string toString(const DfaState &ds) {
  string rv;
  toStringAppend(rv, ds);
  return rv;
}


string toString(const DfaObj &dfa) {
  string rv = "Dfa init=" + to_string(gDfaInitialId) +
    " err=" + to_string(gDfaErrorId) + '\n';
  const vector<DfaState> &vec = dfa.getStates();
  StateId ii = 0;
  for (const DfaState &ds : vec) {
    rv += to_string(ii) + ' ';
    toStringAppend(rv, ds);
    ++ii;
  }
  return rv + "]\n";
}


string toString(const DfaEdge &e) {
  return "edge(" + to_string(e.id_) + ' ' + visibleChar(e.char_) + ')';
}


string toString(const DfaEdgeSet &des) {
  string rv = "edges{\n";
  for (const DfaEdge &e : des)
    rv += "  " + toString(e) + '\n';
  return rv + "}\n";
}


string toString(const DfaEdgeToIds &rev) {
  string rv = "rev{\n";
  for (const auto &[e, sis] : rev)
    rv += "  " + toString(e) + " -> " + toString(sis) + '\n';
  return rv + "}\n";
}


string toString(const BlockRec &br) {
  return "brec(" + to_string(br.block_) + ' ' + visibleChar(br.char_) + ')';
}


string toString(const BlockRecSet &brs) {
  string rv = "brecs{\n";
  for (const BlockRec &br : brs)
    rv += "  " + toString(br) + '\n';
  return rv + "}\n";
}


string toString(const vector<CharIdx> &vec) { // equiv map
  string rv;
  toStringAppend(rv, vec);
  return rv;
}


string toString(const FileHeader &hdr) {
  string rv;
  toStringAppend(rv, hdr);
  return rv;
}


string toString(const char *buf, size_t len) { // serialized
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

  Format fmt = static_cast<Format>(hdr->format_);
  switch (fmt) {
  case fmtOffset1:
  case fmtOffset2:
  case fmtOffset4:
    break;
  default:
    rv = "format not recognized";
    return rv;
  }

  size_t numChars = hdr->maxChar_;
  ++numChars;
  const char *end = buf + len;
  const char *base = buf + sizeof(FileHeader);

  size_t inc;
  switch (fmt) {
  case fmtOffset1:
    inc = DfaProxy<fmtOffset1>::stateSize(hdr->maxChar_);
    break;
  case fmtOffset2:
    inc = DfaProxy<fmtOffset2>::stateSize(hdr->maxChar_);
    break;
  case fmtOffset4:
    inc = DfaProxy<fmtOffset4>::stateSize(hdr->maxChar_);
    break;
  default:
    throw RedExceptInternal("corrupted format");
  }

  for (const char *ptr = base; ptr < end; ptr += inc) {
    size_t off = static_cast<size_t>(ptr - base);
    switch (fmt) {
    case fmtOffset1:
      {
        DfaProxy<fmtOffset1> proxy;
        proxy.init(ptr, 0);
        Result result = proxy.result();
        bool deadEnd = proxy.deadEnd();
        rv += '$' + toHexString(off) + " -> " + to_string(result) + '\n';
        if (deadEnd)
          rv += "  DeadEnd\n";
        for (size_t ii = 0; ii < numChars; ++ii) {
          rv += "  " + to_string(ii) + " -> $" +
            toHexString(proxy.trans(ii)) + '\n';
        }
      }
      break;
    case fmtOffset2:
      {
        DfaProxy<fmtOffset2> proxy;
        proxy.init(ptr, 0);
        Result result = proxy.result();
        bool deadEnd = proxy.deadEnd();
        rv += '$' + toHexString(off) + " -> " + to_string(result) + '\n';
        if (deadEnd)
          rv += "  DeadEnd\n";
        for (size_t ii = 0; ii < numChars; ++ii) {
          rv += "  " + to_string(ii) + " -> $" +
            toHexString(proxy.trans(ii)) + '\n';
        }
      }
      break;
    case fmtOffset4:
      {
        DfaProxy<fmtOffset4> proxy;
        proxy.init(ptr, 0);
        Result result = proxy.result();
        bool deadEnd = proxy.deadEnd();
        rv += '$' + toHexString(off) + " -> " + to_string(result) + '\n';
        if (deadEnd)
          rv += "  DeadEnd\n";
        for (size_t ii = 0; ii < numChars; ++ii) {
          rv += "  " + to_string(ii) + " -> $" +
            toHexString(proxy.trans(ii)) + '\n';
        }
      }
      break;
    default:
      break;
    }
  }

  return rv + "END\n";
}


char toHexDigit(Byte x) {
  if (x <= 9)
    return '0' + static_cast<char>(x);
  return static_cast<char>('a' - 10 + x);
}

///////////////////////////////////////////////////////////////////////////////

ostream &operator<<(ostream &os, const MultiChar &mc) {
  os << toString(mc);
  return os;
}

ostream &operator<<(ostream &os, const StateIdSet &sis) {
  os << toString(sis); // FIXME: non-char version
  return os;
}

} // namespace zezax::red
