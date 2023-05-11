// sensor.cpp - c++ wrapper of lm-sensors api

#include "sensor.h"

#include <cstring>

#include <algorithm>
#include <stdexcept>

#include "util.h"

namespace zezax::sensorrd {

using std::make_unique;
using std::make_shared;
using std::runtime_error;
using std::string;
using std::string_view;


ContextT::ContextT()
  : ok_(false) {
  int err = sensors_init(nullptr);
  if (err)
    throw runtime_error("failed to init sensors");
  ok_ = true;
}


ContextT::~ContextT() {
  if (ok_) {
    sensors_cleanup();
    ok_ = false;
  }
}

///////////////////////////////////////////////////////////////////////////////

ChipIterT::ChipIterT(ContextPtrT ctx)
  : ctx_(std::move(ctx)), idx_(0), chip_(nullptr) {
  ++*this;
}


ChipIterT &ChipIterT::operator++() {
  chip_ = sensors_get_detected_chips(nullptr, &idx_);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

FeatureIterT::FeatureIterT(ContextPtrT ctx, const sensors_chip_name *chip)
  : ctx_(std::move(ctx)),
    chip_(chip),
    idx_(0),
    feature_(nullptr),
    label_(nullptr) {
  ++*this;
}


FeatureIterT::~FeatureIterT() {
  cleanup();
}


FeatureIterT &FeatureIterT::operator++() {
  for (;;) {
    cleanup();
    feature_ = sensors_get_features(chip_, &idx_);
    if (!feature_)
      break;
    label_ = sensors_get_label(chip_, feature_);
    if (label_)
      break;
  }
  return *this;
}


void FeatureIterT::cleanup() {
  if (label_) {
    free((void *) label_);
    label_ = nullptr;
  }
}

///////////////////////////////////////////////////////////////////////////////

SubIterT::SubIterT(ContextPtrT              ctx,
                   const sensors_chip_name *chip,
                   const sensors_feature   *feature)
  : ctx_(std::move(ctx)),
    chip_(chip),
    feature_(feature),
    idx_(0),
    sub_(nullptr),
    val_(0.0) {
  ++*this;
}


SubIterT &SubIterT::operator++() {
  for (;;) {
    sub_ = sensors_get_all_subfeatures(chip_, feature_, &idx_);
    if (!sub_ ||
        ((sub_->flags & SENSORS_MODE_R) &&
         (sensors_get_value(chip_, sub_->number, &val_) == 0)))
      break;
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

struct ChipCmpT {
  bool operator()(const sensors_chip_name *aa,
                  const sensors_chip_name *bb) {
    // reaching in under the covers a bit...
    if (aa->bus.type < bb->bus.type) // primary sort: bus type
      return true;
    if (aa->bus.type > bb->bus.type)
      return false;

    if (aa->bus.nr < bb->bus.nr) // secondard sort: bus number
      return true;
    if (aa->bus.nr > bb->bus.nr)
      return false;

    int cmp = strcmp(aa->prefix, bb->prefix); // tertiary sort: name
    if (cmp < 0)
      return true;
    if (cmp > 0)
      return true;

    return (aa->addr < bb->addr); // final sort: address
  }
};


AllIterT::AllIterT(ContextPtrT ctx)
  : ctx_(std::move(ctx)),
    cIdx_(0),
    fIdx_(0),
    sIdx_(0),
    chip_(nullptr),
    feature_(nullptr),
    sub_(nullptr),
    label_(nullptr),
    val_(0.0) {
  ++*this;
}


AllIterT::AllIterT(const AllIterT &other)
  : ctx_(other.ctx_),
    cIdx_(other.cIdx_),
    fIdx_(other.fIdx_),
    sIdx_(other.sIdx_),
    chip_(other.chip_),
    feature_(other.feature_),
    sub_(other.sub_),
    label_(nullptr),
    val_(other.val_),
    chipAry_(other.chipAry_) {
  if (other.label_) {
    label_ = sensors_get_label(chip_, feature_);
    if (!label_)
      throw std::bad_alloc();
  }
}


AllIterT::~AllIterT() {
  clearLabel();
}


AllIterT &AllIterT::operator=(const AllIterT &rhs) {
  if (this == &rhs)
    return *this;
  cIdx_    = rhs.cIdx_;
  fIdx_    = rhs.fIdx_;
  sIdx_    = rhs.sIdx_;
  chip_    = rhs.chip_;
  feature_ = rhs.feature_;
  sub_     = rhs.sub_;
  val_     = rhs.val_;
  chipAry_ = rhs.chipAry_;
  if (!label_ || !rhs.label_ || strcmp(label_, rhs.label_)) {
    clearLabel();
    if (rhs.label_) {
      label_ = sensors_get_label(chip_, feature_);
      if (!label_)
        throw std::bad_alloc();
    }
  }
  return *this;
}


AllIterT &AllIterT::operator++() {
  // for determinism, we need the chips in sorted order
  if (!chipAry_) {
    chipAry_ = make_shared<ChipVecT>();
    int nr = 0;
    const sensors_chip_name *scn;
    while ((scn = sensors_get_detected_chips(nullptr, &nr)))
      chipAry_->push_back(scn);
    ChipCmpT cmp;
    std::sort(chipAry_->begin(), chipAry_->end(), cmp);
  }

  for (;;) { // sub
    if (feature_) {
      for (;;) {
        sub_ = sensors_get_all_subfeatures(chip_, feature_, &sIdx_);
        if (!sub_ ||
            ((sub_->flags & SENSORS_MODE_R) &&
             (sensors_get_value(chip_, sub_->number, &val_) == 0)))
          break;
      }
      if (sub_)
        break;
    }

    for (;;) { // feature
      if (chip_) {
        for (;;) {
          clearLabel();
          feature_ = sensors_get_features(chip_, &fIdx_);
          if (!feature_)
            break;
          label_ = sensors_get_label(chip_, feature_);
          if (label_)
            break;
        }
        if (feature_) {
          sIdx_ = 0;
          break;
        }
      }

      // chip
      if (cIdx_ >= static_cast<int>(chipAry_->size()))
        return *this; // gotta exit, no more chips
      chip_ = (*chipAry_)[cIdx_++];
      fIdx_ = 0;
    }
  }

  return *this;
}


void AllIterT::clearLabel() {
  if (label_) {
    free((void *) label_);
    label_ = nullptr;
  }
}

///////////////////////////////////////////////////////////////////////////////

TupleIterT::TupleIterT(ContextPtrT ctx)
  : iter_(ctx), // should be move
    prevIter_(iter_),
    cIdx_(-1),
    fIdx_(-1),
    alarm_(false),
    prevAlarm_(false),
    val_(0.0),
    prevVal_(0.0) {
  ++*this;
}


TupleIterT &TupleIterT::operator++() {
  for (;;) {
    prevIter_ = iter_;
    prevAlarm_ = alarm_;
    prevVal_ = val_;
    bool first = (cIdx_ < 0);
    if (!first)
      ++iter_;
    if (!iter_)
      break;

    string_view pfx;
    string_view suf;
    split(iter_.getName(), '_', pfx, suf);
    if (suf == "_input")
      val_ = iter_.getVal();
    else if (suf == "_alarm")
      alarm_ = (iter_.getVal() > 0.0);

    int cNew = iter_.getChipIdx();
    int fNew = iter_.getFeatureIdx();
    if ((cIdx_ != cNew) || (fIdx_ != fNew)) {
      cIdx_ = cNew;
      fIdx_ = fNew;
      if (!first)
        break;
    }
  }
  return *this;
}


TupleT TupleIterT::getTuple() const {
  string_view pfx;
  string_view suf;
  split(prevIter_.getName(), '_', pfx, suf);
  TupleT rv;
  rv.chip_ = ChipT::toString(prevIter_.getChip());
  rv.label_ = prevIter_.getLabel();
  rv.name_ = pfx;
  rv.val_ = prevVal_;
  rv.alarm_ = prevAlarm_;
  return rv;
}

///////////////////////////////////////////////////////////////////////////////

ChipT::ChipT(ContextPtrT ctx, const sensors_chip_name *chip)
  : ctx_(std::move(ctx)), chip_(chip) {}


string ChipT::toString() const {
  return toString(chip_);
}


/* static */ string ChipT::toString(const sensors_chip_name *chip) {
  char buf[1024];
  int res = sensors_snprintf_chip_name(buf, sizeof(buf), chip);
  if (res < 0)
    throw runtime_error("failed to format chip name");
  return buf;
}

} // namespace zezax::sensorrd
