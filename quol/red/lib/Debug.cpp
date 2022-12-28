// general printing and debugging implementation

#include "Debug.h"

#include <stdexcept>
#include <algorithm>
#include <set>
#include <vector>

namespace zezax::red {

using std::logic_error;
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
  throw logic_error("bad token type");
}


string toString(const NfaObj &nfa) {
  string rv;
  NfaId n = static_cast<NfaId>(nfa.size());
  for (NfaId id = 0; id < n; ++id) {
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


string toString(const vector<CharIdx> &vec) {
  string rv = "equiv[\n";
  CharIdx start = 0;
  CharIdx prev = ~0U;
  CharIdx n = static_cast<CharIdx>(vec.size());
  CharIdx idx;
  for (idx = 0; idx < n; ++idx) {
    CharIdx ch = vec[idx];
    if (ch != prev) {
      if (prev != ~0U) {
        rv += "  " + to_string(prev) + " <- " + to_string(start);
        if ((idx - 1) > start) {
          rv += '-';
          rv += to_string(idx - 1);
        }
        rv += '\n';
      }
      start = idx;
    }
    prev = ch;
  }
  if (prev != ~0U) {
    --idx;
    rv += "  " + to_string(prev) + " <- " + to_string(start);
    if (idx > start) {
      rv += '-';
      rv += to_string(idx);
    }
    rv += '\n';
  }
  return rv + "]\n";
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
