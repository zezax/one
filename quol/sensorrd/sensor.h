// sensor.h - c++ wrapper of lm-sensors api

#pragma once

#include <sensors/sensors.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace zezax::sensorrd {

class ContextT {
public:
  ContextT();
  ~ContextT();

private:
  bool ok_;
};


typedef std::shared_ptr<ContextT> ContextPtrT;
typedef std::vector<const sensors_chip_name *> ChipVecT;
typedef std::shared_ptr<ChipVecT> ChipVecPtrT;


class ChipIterT {
public:
  ChipIterT(ContextPtrT ctx);
  ChipIterT &operator++();
  explicit operator bool() const { return (chip_ != nullptr); }
  const sensors_chip_name *getChip() const { return chip_; }

private:
  ContextPtrT              ctx_;
  int                      idx_;
  const sensors_chip_name *chip_;
};


class FeatureIterT {
public:
  FeatureIterT(ContextPtrT ctx, const sensors_chip_name *chip);
  ~FeatureIterT();
  FeatureIterT &operator++();
  explicit operator bool() const { return (feature_ != nullptr); }

  const sensors_chip_name *getChip() const { return chip_; }
  const sensors_feature *getFeature() const { return feature_; }
  std::string_view getLabel() const { return label_; }

private:
  ContextPtrT              ctx_;
  const sensors_chip_name *chip_;
  int                      idx_;
  const sensors_feature   *feature_;
  const char              *label_;

  void cleanup();
};


class SubIterT {
public:
  SubIterT(ContextPtrT              ctx,
           const sensors_chip_name *chip,
           const sensors_feature   *feature);
  SubIterT &operator++();
  explicit operator bool() const { return (sub_ != nullptr); }

  const sensors_chip_name *getChip() const { return chip_; }
  const sensors_feature *getFeature() const { return feature_; }
  const sensors_subfeature *getSub() const { return sub_; }
  std::string_view getName() const { return sub_->name; }
  double getVal() const { return val_; }

private:
  ContextPtrT               ctx_;
  const sensors_chip_name  *chip_;
  const sensors_feature    *feature_;
  int                       idx_;
  const sensors_subfeature *sub_;
  double                    val_;
};


class AllIterT {
public:
  AllIterT(ContextPtrT ctx);
  AllIterT(const AllIterT &other);
  ~AllIterT();
  AllIterT &operator=(const AllIterT &rhs);
  AllIterT &operator++();
  explicit operator bool() const { return (sub_ != nullptr); }

  int getChipIdx() const { return cIdx_; }
  int getFeatureIdx() const { return fIdx_; }
  int getSubIdx() const { return sIdx_; }
  const sensors_chip_name *getChip() const { return chip_; }
  const sensors_feature *getFeature() const { return feature_; }
  const sensors_subfeature *getSub() const { return sub_; }
  std::string_view getLabel() const { return label_; }
  std::string_view getName() const { return sub_->name; }
  double getVal() const { return val_; }

private:
  void clearLabel();

  ContextPtrT               ctx_;
  int                       cIdx_;
  int                       fIdx_;
  int                       sIdx_;
  const sensors_chip_name  *chip_;
  const sensors_feature    *feature_;
  const sensors_subfeature *sub_;
  const char               *label_;
  double                    val_;
  ChipVecPtrT               chipAry_;
};


struct TupleT {
  std::string chip_;
  std::string label_;
  std::string name_;
  double      val_;
  bool        alarm_;
};


class TupleIterT {
public:
  TupleIterT(ContextPtrT ctx);
  TupleIterT &operator++();
  explicit operator bool() const { return (iter_ || prevIter_); }

  TupleT getTuple() const;

private:
  AllIterT iter_;
  AllIterT prevIter_;
  int      cIdx_;
  int      fIdx_;
  bool     alarm_;
  bool     prevAlarm_;
  double   val_;
  double   prevVal_;
};


class ChipT {
public:
  ChipT(ContextPtrT ctx, const sensors_chip_name *chip);
  std::string toString() const;
  static std::string toString(const sensors_chip_name *chip);

private:
  ContextPtrT              ctx_;
  const sensors_chip_name *chip_;
};

} // namespace zezax::sensorrd
