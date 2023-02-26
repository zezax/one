// non-deterministic finite automaton object implementation

#include "Nfa.h"

#include <limits>
#include <utility>

#include "Except.h"
#include "Fnv.h"

#include <unordered_map>
#include <unordered_set>

namespace zezax::red {

using std::numeric_limits;
using std::unordered_map;
using std::vector;

namespace {

bool containsTr(const vector<NfaTransition> &vec, const NfaTransition &tr) {
  for (const NfaTransition &elem : vec)
    if (elem == tr)
      return true;
  return false;
}


void addTransition(NfaState &ns, const NfaTransition &tr) {
  if (!containsTr(ns.transitions_, tr))
    ns.transitions_.push_back(tr);
}


void addTransition(NfaState &ns, NfaTransition &&tr) {
  if (!containsTr(ns.transitions_, tr))
    ns.transitions_.emplace_back(std::move(tr));
}


bool stateAccepts(const NfaState &ns) {
  return (ns.result_ > 0);
}


void allStatesRecurse(NfaIdSet &out, NfaId id, const vector<NfaState> &states) {
  for (const NfaTransition &tr : states[id].transitions_)
    if (!out.testAndSet(tr.next_))
      allStatesRecurse(out, tr.next_, states);
}


void allMultiCharsRecurse(MultiCharSet           &out,
                          NfaIdSet               &seen,
                          NfaId                   id,
                          const vector<NfaState> &states) {
  for (const NfaTransition &tr : states[id].transitions_) {
    out.emplace(tr.multiChar_);
    if (!seen.testAndSet(tr.next_))
      allMultiCharsRecurse(out, seen, tr.next_, states);
  }
}

} // anonymous

NfaObj::NfaObj(NfaObj &&rhs)
  : states_(std::move(rhs.states_)),
    initId_(std::exchange(rhs.initId_, gNfaNullId)),
    goal_(std::exchange(rhs.goal_, 0)) {}


NfaObj &NfaObj::operator=(NfaObj &&rhs) {
  states_ = std::move(rhs.states_);
  initId_ = std::exchange(rhs.initId_, gNfaNullId);
  goal_   = std::exchange(rhs.goal_, 0);
  return *this;
}


size_t NfaObj::activeStates() const {
  NfaIdSet all = allStates(initId_);
  return all.size();
}


void NfaObj::freeAll() {
  states_.clear();
  states_.shrink_to_fit();
}


NfaId NfaObj::newState(Result result) {
  size_t len = states_.size();
  if (len == 0)
    len = 1; // zero is invalid state id
  if (len > numeric_limits<NfaId>::max())
    throw RedExceptLimit("nfa state id overflow");
  states_.resize(len + 1);
  NfaState &ns = states_[len];
  ns.result_ = result;
  return static_cast<NfaId>(len);
}


NfaId NfaObj::deepCopyState(NfaId id) {
  if (!id)
    return id;
  unordered_map<NfaId, NfaId> map;
  return copyRecurse(map, id);
}


bool NfaObj::accepts(NfaId id) const {
  return stateAccepts(states_[id]);
}


bool NfaObj::hasAccept(const NfaIdSet &nis) const {
  for (NfaId id : nis)
    if (accepts(id))
      return true;
  return false;
}


NfaIdSet NfaObj::allStates(NfaId id) const {
  NfaIdSet rv;
  if (id) {
    rv.insert(id);
    allStatesRecurse(rv, id, states_);
  }
  return rv;
}


MultiCharSet NfaObj::allMultiChars(NfaId id) const {
  MultiCharSet rv;
  NfaIdSet seen;
  if (id) {
    seen.insert(id);
    allMultiCharsRecurse(rv, seen, id, states_);
  }
  return rv;
}


vector<NfaId> NfaObj::allAcceptingStates(NfaId id) const {
  vector<NfaId> rv;
  for (NfaId state : allStates(id))
    if (accepts(state))
      rv.push_back(state);
  return rv;
}


vector<NfaStateTransition> NfaObj::allAcceptingTransitions(NfaId id) const {
  vector<NfaStateTransition> rv;
  for (NfaId state : allStates(id))
    for (const NfaTransition &trans : states_[state].transitions_)
      if (accepts(trans.next_)) {
        NfaStateTransition nst{state, trans};
        rv.emplace_back(std::move(nst));
      }
  return rv;
}


// combine both functions above with one call to allStates() and one iteration
void NfaObj::allAcceptingStatesTransitions(
    NfaId                       id,
    vector<NfaId>              &accStates,
    vector<NfaStateTransition> &accTrans) const {
  for (NfaId state : allStates(id)) {
    if (accepts(state))
      accStates.push_back(state);
    for (const NfaTransition &trans : states_[state].transitions_)
      if (accepts(trans.next_)) {
        NfaStateTransition nst{state, trans};
        accTrans.emplace_back(std::move(nst));
      }
  }
}


NfaId NfaObj::stateUnion(NfaId xx, NfaId yy) {
  NfaState &sxx = states_[xx];
  const NfaState &syy = states_[yy];
  if (!stateAccepts(sxx) && stateAccepts(syy))
    sxx.result_ = syy.result_;
  for (const NfaTransition &syyTr : syy.transitions_)
    addTransition(sxx, syyTr);
  return xx;
}


NfaId NfaObj::stateConcat(NfaId xx, NfaId yy) {
  if (!xx)
    return yy;
  if (!yy)
    return xx;

  vector<NfaId> accStates;
  vector<NfaStateTransition> accTrans;
  allAcceptingStatesTransitions(xx, accStates, accTrans);

  for (NfaStateTransition &strans : accTrans) {
    NfaTransition tr{yy, strans.transition_.multiChar_}; // FIXME
    addTransition(states_[strans.state_], std::move(tr));
  }

  NfaState &sxx = states_[xx];
  const NfaState &syy = states_[yy];

  if (stateAccepts(sxx))
    for (const NfaTransition &syyTr : syy.transitions_)
      addTransition(sxx, syyTr);

  Result keepResult = 0;
  if (stateAccepts(sxx) && stateAccepts(syy))
    keepResult = sxx.result_;

  for (NfaId id : accStates)
    states_[id].result_ = 0; // no longer accepting

  if (keepResult > 0)
    sxx.result_ = keepResult;

  return xx;
}


NfaId NfaObj::stateOptional(NfaId id) {
  states_[id].result_ = goal_;
  return id;
}


NfaId NfaObj::stateKleenStar(NfaId id) {
  vector<NfaStateTransition> accTrans = allAcceptingTransitions(id);
  for (NfaStateTransition &strans : accTrans) {
    NfaTransition tr{id, strans.transition_.multiChar_}; //FIXME
    addTransition(states_[strans.state_], std::move(tr));
  }
  return stateOptional(id);
}


NfaId NfaObj::stateClosure(NfaId id, int min, int max) {
  if (min == 0) {
    if (max < 0)
      return stateKleenStar(id);
    NfaId option = gNfaNullId;
    for (int ii = 1; ii < max; ++ii)
      option = stateConcat(option, stateOptional(deepCopyState(id)));
    return stateConcat(stateOptional(id), option);
  }

  if (min < 0)
    min = max;
  NfaId caboose = gNfaNullId;
  if (max < 0)
    caboose = stateKleenStar(deepCopyState(id));
  else {
    int num = max - min;
    for (int ii = 0; ii < num; ++ii)
      caboose = stateConcat(caboose, stateOptional(deepCopyState(id)));
  }

  NfaId front = gNfaNullId;
  for (int ii = 1; ii < min; ++ii)
    front = stateConcat(front, deepCopyState(id));
  front = stateConcat(id, front);
  return stateConcat(front, caboose);
}


NfaId NfaObj::stateIgnoreCase(NfaId id) {
  for (NfaId state : allStates(id)) {
    for (NfaTransition &trans : states_[state].transitions_) {
      for (Byte uc = 'A'; uc <= 'Z'; ++uc)
        if (trans.multiChar_.get(uc))
          trans.multiChar_.insert(uc + 32); // lower-case in ascii
      for (Byte uc = 'a'; uc <= 'z'; ++uc)
        if (trans.multiChar_.get(uc))
          trans.multiChar_.insert(uc - 32); // upper-case in ascii
    }
  }
  return id;
}


NfaId NfaObj::stateWildcard() { // basically dot-star
  MultiChar mc;
  mc.resize(gAlphabetSize);
  mc.setAll();
  NfaId init = newState(0);
  NfaId goal = newGoalState();
  NfaTransition tr{goal, mc}; // FIXME constructor
  states_[init].transitions_.emplace_back(std::move(tr));
  return stateKleenStar(init);
}


NfaId NfaObj::stateEndMark(CharIdx res) {
  CharIdx x = res + gAlphabetSize;
  if ((x < res) || (x < gAlphabetSize))
    throw RedExceptLimit("end-mark overflow");
  MultiChar mc;
  mc.insert(x);
  NfaId init = newState(0);
  NfaId goal = newGoalState();
  NfaTransition tr{goal, mc}; // FIXME constructor
  states_[init].transitions_.emplace_back(std::move(tr));
  return init;
}


void NfaObj::selfUnion(NfaId id) {
  if (initId_)
    initId_ = stateUnion(initId_, id);
  else
    initId_ = id;
}


void NfaObj::dropUselessTransitions() { // also de-dup transitions
  // identify useless states
  NfaIdSet useless;
  NfaId num = static_cast<NfaId>(states_.size());
  for (NfaId id = 1; id < num; ++id) {
    NfaState &ns = states_[id];
    if ((ns.result_ <= 0) && ns.transitions_.empty())
      useless.insert(id);
  }

  // rewrite transitions without useless ones
  for (NfaState &ns : states_) {
    vector<NfaTransition> newTrs;
    for (NfaTransition &tr : ns.transitions_)
      if (!useless.get(tr.next_) && !containsTr(newTrs, tr))
        newTrs.emplace_back(std::move(tr));
    ns.transitions_.swap(newTrs);
  }
}

///////////////////////////////////////////////////////////////////////////////

NfaId NfaObj::copyRecurse(unordered_map<NfaId, NfaId> &map, NfaId id) {
  auto [it, novel] = map.insert({id, 0}); // FIXME ugly
  if (novel) {
    NfaId newId = newState(states_[id].result_);
    it->second = newId;
    for (const NfaTransition &tr : states_[id].transitions_) {
      NfaId subId = copyRecurse(map, tr.next_);
      auto &newTrs = states_[newId].transitions_;
      newTrs.push_back(tr);
      newTrs.back().next_ = subId;
    }
    return newId;
  }
  return it->second;
}

} // namespace zezax::red
