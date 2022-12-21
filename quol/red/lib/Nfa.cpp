// non-deterministic finite automaton implementation

#include "Nfa.h"

#include "Except.h"
#include "Fnv.h"

#include <unordered_map>
#include <unordered_set>

namespace zezax::red {

using std::unordered_map;
using std::unordered_set;
using std::vector;

namespace {

void allStatesRecurse(NfaStateSet &out, NfaState *ns) {
  if (!ns)
    return;
  auto [_, novel] = out.insert(ns);
  if (!novel)
    return;

  for (const NfaTransition &tr : ns->transitions_)
    allStatesRecurse(out, tr.next_);
}


void allMultiCharsRecurse(MultiCharSet &out,
                          NfaStateSet &seen,
                          NfaState *ns) {
  if (!ns)
    return;
  auto [_, novel] = seen.insert(ns);
  if (!novel)
    return;

  for (const NfaTransition &tr : ns->transitions_) {
    out.emplace(tr.multiChar_);
    allMultiCharsRecurse(out, seen, tr.next_);
  }
}


NfaState *copyRecurse(NfaObj &obj,
                      unordered_map<NfaState *, NfaState *> &map,
                      NfaState *ns) {
  if (!ns)
    return nullptr;

  auto [it, novel] = map.insert({ns, nullptr}); // FIXME ugly
  if (novel) {
    NfaState *state = obj.newState(ns->result_);
    it->second = state;
    for (NfaTransition &tr : ns->transitions_) {
      state->transitions_.push_back(tr);
      state->transitions_.back().next_ = copyRecurse(obj, map, tr.next_);
    }
    return state;
  }
  else
    return it->second;
}

} // anonymous

bool accepts(const NfaState *ns) {
  return (ns->result_ > 0);
}


bool hasAccept(const NfaStateSet &ss) {
  for (const NfaState *ns : ss)
    if (accepts(ns))
      return true;
  return false;
}


size_t hashState(const NfaState *ns) {
  return fnv1a<size_t>(&ns->id_, sizeof(ns->id_)); // should uniquely identify
}


NfaStateSet allStates(NfaState *ns) {
  NfaStateSet rv;
  allStatesRecurse(rv, ns);
  return rv;
}


vector<NfaState *> allAcceptingStates(NfaState *ns) {
  vector<NfaState *> rv;
  for (NfaState *state : allStates(ns)) {
    if (accepts(state))
      rv.push_back(state);
  }
  return rv;
}


vector<NfaStateTransition> allAcceptingTransitions(NfaState *ns) {
  vector<NfaStateTransition> rv;
  for (NfaState *state : allStates(ns))
    for (NfaTransition &trans : state->transitions_)
      if (accepts(trans.next_)) {
        NfaStateTransition nst{state, trans};
        rv.emplace_back(std::move(nst));
      }
  return rv;
}


MultiCharSet allMultiChars(NfaState *ns) {
  MultiCharSet rv;
  NfaStateSet seen;
  allMultiCharsRecurse(rv, seen, ns);
  return rv;
}

///////////////////////////////////////////////////////////////////////////////

NfaObj::NfaObj(NfaObj &&rhs)
: nfa_(rhs.nfa_), alloc_(rhs.alloc_), curId_(rhs.curId_), goal_(rhs.goal_) {
  rhs.reset();
}


NfaObj &NfaObj::operator=(NfaObj &&rhs) {
  nfa_ = rhs.nfa_;
  alloc_ = rhs.alloc_;
  curId_ = rhs.curId_;
  goal_ = rhs.goal_;
  rhs.reset();
  return *this;
}


void NfaObj::freeAll() {
  NfaState *ptr = alloc_;
  while (ptr) {
    NfaState *next = ptr->allocNext_;
    delete ptr;
    ptr = next;
  }
}


NfaState *NfaObj::newState(Result result) {
  NfaState *rv = new NfaState();
  rv->id_ = ++curId_;
  rv->result_ = result;
  rv->allocNext_ = alloc_;
  alloc_ = rv;
  return rv;
}


NfaState *NfaObj::deepCopyState(NfaState *ns) {
  unordered_map<NfaState *, NfaState *> map;
  return copyRecurse(*this, map, ns);
}


NfaState *NfaObj::stateUnion(NfaState *xx, NfaState *yy) {
  if (!accepts(xx) && accepts(yy))
    xx->result_ = yy->result_;
  xx->transitions_.insert(xx->transitions_.end(),
                          yy->transitions_.begin(),
                          yy->transitions_.end());
  return xx;
}


NfaState *NfaObj::stateConcat(NfaState *xx, NfaState *yy) {
  if (!xx)
    return yy;
  if (!yy)
    return xx;

  vector<NfaStateTransition> accTrans = allAcceptingTransitions(xx);
  vector<NfaState *> accStates = allAcceptingStates(xx);

  for (NfaStateTransition &strans : accTrans) {
    NfaTransition tr{yy, strans.transition_.multiChar_}; // FIXME
    strans.state_->transitions_.emplace_back(std::move(tr));
  }

  if (accepts(xx))
    xx->transitions_.insert(xx->transitions_.end(),
                            yy->transitions_.begin(),
                            yy->transitions_.end());

  Result keepResult = 0;
  if (accepts(xx) && accepts(yy))
    keepResult = xx->result_;

  for (NfaState *state : accStates)
    state->result_ = 0; // no longer accepting

  if (keepResult > 0)
    xx->result_ = keepResult;

  return xx;
}


NfaState *NfaObj::stateOptional(NfaState *ns) {
  ns->result_ = goal_;
  return ns;
}


NfaState *NfaObj::stateKleenStar(NfaState *ns) {
  vector<NfaStateTransition> accTrans = allAcceptingTransitions(ns);
  for (NfaStateTransition &strans : accTrans) {
    NfaTransition tr{ns, strans.transition_.multiChar_}; //FIXME
    strans.state_->transitions_.emplace_back(std::move(tr));
  }
  return stateOptional(ns);
}


NfaState *NfaObj::stateOneOrMore(NfaState *ns) {
  return stateConcat(ns, stateKleenStar(deepCopyState(ns)));
}


NfaState *NfaObj::stateCount(NfaState *ns, int min, int max) {
  if (min == 0) {
    if (max < 0)
      return stateKleenStar(ns);
    NfaState *option = nullptr;
    for (int ii = 1; ii < max; ++ii)
      option = stateConcat(option, stateOptional(deepCopyState(ns)));
    return stateConcat(stateOptional(ns), option);
  }

  if (min < 0)
    min = max;
  NfaState *caboose = nullptr;
  if (max < 0)
    caboose = stateKleenStar(deepCopyState(ns));
  else {
    int num = max - min;
    for (int ii = 0; ii < num; ++ii)
      caboose = stateConcat(caboose, stateOptional(deepCopyState(ns)));
  }

  NfaState *front = nullptr;
  for (int ii = 1; ii < min; ++ii)
    front = stateConcat(front, deepCopyState(ns));
  front = stateConcat(ns, front);
  return stateConcat(front, caboose);
}


NfaState *NfaObj::stateIgnoreCase(NfaState *ns) {
  for (NfaState *state : allStates(ns)) {
    for (NfaTransition &trans : state->transitions_) {
      for (Byte uc = 'A'; uc <= 'Z'; ++uc)
        if (trans.multiChar_.get(uc))
          trans.multiChar_.set(uc + 32); // lower-case in ascii
      for (Byte uc = 'a'; uc <= 'z'; ++uc)
        if (trans.multiChar_.get(uc))
          trans.multiChar_.set(uc - 32); // upper-case in ascii
    }
  }
  return ns;
}


NfaState *NfaObj::stateWildcard() { // basically .*
  MultiChar mc;
  mc.resize(gAlphabetSize);
  mc.setAll();
  NfaState *init = newState(0);
  NfaState *goal = newGoalState();
  NfaTransition tr{goal, mc}; // FIXME constructor
  init->transitions_.emplace_back(std::move(tr));
  return stateKleenStar(init);
}


NfaState *NfaObj::stateEndMark(CharIdx res) {
  CharIdx x = res + gAlphabetSize;
  if ((x < res) || (x < gAlphabetSize))
    throw RedExcept("end-mark overflow");
  MultiChar mc;
  mc.set(x);
  NfaState *init = newState(0);
  NfaState *goal = newGoalState();
  NfaTransition tr{goal, mc}; // FIXME constructor
  init->transitions_.emplace_back(std::move(tr));
  return init;
}


void NfaObj::selfUnion(NfaState *ns) {
  if (nfa_)
    nfa_ = stateUnion(nfa_, ns);
  else
    nfa_ = ns;
}


void NfaObj::reset() {
  nfa_ = nullptr;
  alloc_ = nullptr;
  curId_ = 0;
  goal_ = 0;
}

} // namespace zezax::red
