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
  : ctx_(ctx),
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

///////////////////////////////////////////////////////////////////////////////

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


AllIterT &AllIterT::operator++() {
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
      chip_ = sensors_get_detected_chips(nullptr, &cIdx_);
      if (!chip_)
        return *this; // gotta exit, no more chips
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
