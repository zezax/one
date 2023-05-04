// wrapper.cpp - c++ wrapper of lm-sensors api

#include "wrapper.h"

#include <stdexcept>
#include <iostream> // FIXME

namespace zezax::sensorrd {

using std::make_unique;
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
  : ctx_(std::move(ctx)), idx_(0), cname_(nullptr) {
  ++*this;
}


ChipIterT &ChipIterT::operator++() {
  cname_ = sensors_get_detected_chips(nullptr, &idx_);
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

FeatureIterT::FeatureIterT(ContextPtrT ctx, const sensors_chip_name *cname)
  : ctx_(std::move(ctx)),
    cname_(cname),
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
    feature_ = sensors_get_features(cname_, &idx_);
    if (!feature_)
      break;
    label_ = sensors_get_label(cname_, feature_);
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
                   const sensors_chip_name *cname,
                   const sensors_feature   *feature)
  : ctx_(ctx),
    cname_(cname),
    feature_(feature),
    idx_(0),
    sub_(nullptr),
    val_(0.0) {
  ++*this;
}


SubIterT &SubIterT::operator++() {
  for (;;) {
    sub_ = sensors_get_all_subfeatures(cname_, feature_, &idx_);
    if (!sub_ ||
        ((sub_->flags & SENSORS_MODE_R) &&
         (sensors_get_value(cname_, sub_->number, &val_) == 0)))
      break;
  }
  return *this;
}

///////////////////////////////////////////////////////////////////////////////

ChipT::ChipT(ContextPtrT ctx, const sensors_chip_name *cname)
  : ctx_(std::move(ctx)), cname_(cname) {}


string ChipT::toString() const {
  return toString(cname_);
}


/* static */ string ChipT::toString(const sensors_chip_name *cname) {
  char buf[1024];
  int res = sensors_snprintf_chip_name(buf, sizeof(buf), cname);
  if (res < 0)
    throw runtime_error("failed to format chip name");
  return buf;
}

///////////////////////////////////////////////////////////////////////////////

AllIterT::AllIterT(ContextPtrT ctx)
  : ctx_(std::move(ctx)),
    cIter_(ctx_),
    cPos_(-1),
    fPos_(-1),
    sPos_(-1) {
  ++*this;
}


AllIterT &AllIterT::operator++() {
  for (;;) { // sub
    if (sIter_) {
      if (++sPos_ > 0)
        ++*sIter_;
      if (*sIter_)
        break;
      sIter_.reset(nullptr); // to signal end of iteration
    }

    for (;;) { // feature
      if (fIter_) {
        if (++fPos_ > 0)
          ++*fIter_;
        if (*fIter_) {
          sPos_ = -1;
          sIter_ = make_unique<SubIterT>(ctx_, fIter_->getChip(),
                                         fIter_->getFeature());
          break;
        }
      }

      // chip
      if (++cPos_ > 0)
        ++cIter_;
      if (cIter_) {
        fIter_ = make_unique<FeatureIterT>(ctx_, cIter_.getChip());
        fPos_ = -1;
      }
      else
        return *this; // gotta exit, no more chips
    }
  }

  return *this;
}

///////////////////////////////////////////////////////////////////////////////

TupleIterT::TupleIterT(ContextPtrT ctx)
  : iter_(std::move(ctx)), val_(0.0), alarm_(false) {
  ++*this;
}


TupleIterT &TupleIterT::operator++() {
  for (;;) {
    if (!key_.empty())
      ++iter_;
    if (!iter_)
      break;
    string_view name = iter_.getName();
    key_ = name;
    break;
  }
  return *this;
}

} // namespace zezax::sensorrd
