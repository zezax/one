// wrapper.h - c++ wrapper of lm-sensors api

#pragma once

#include <sensors/sensors.h>

#include <memory>
#include <string>
#include <string_view>

namespace zezax::sensorrd {

class ContextT {
public:
  ContextT();
  ~ContextT();

private:
  bool ok_;
};


typedef std::shared_ptr<ContextT> ContextPtrT;


class ChipIterT {
public:
  ChipIterT(ContextPtrT ctx);
  ChipIterT &operator++();
  explicit operator bool() const { return (cname_ != nullptr); }
  const sensors_chip_name *getChip() const { return cname_; }

private:
  ContextPtrT              ctx_;
  int                      idx_;
  const sensors_chip_name *cname_;
};


class FeatureIterT {
public:
  FeatureIterT(ContextPtrT ctx, const sensors_chip_name *cname);
  ~FeatureIterT();
  FeatureIterT &operator++();
  explicit operator bool() const { return (feature_ != nullptr); }

  const sensors_chip_name *getChip() const { return cname_; }
  const sensors_feature *getFeature() const { return feature_; }
  std::string_view getLabel() const { return label_; }

private:
  ContextPtrT              ctx_;
  const sensors_chip_name *cname_;
  int                      idx_;
  const sensors_feature   *feature_;
  const char              *label_;

  void cleanup();
};


class SubIterT {
public:
  SubIterT(ContextPtrT              ctx,
           const sensors_chip_name *cname,
           const sensors_feature   *feature);
  SubIterT &operator++();
  explicit operator bool() const { return (sub_ != nullptr); }

  const sensors_chip_name *getChip() const { return cname_; }
  const sensors_feature *getFeature() const { return feature_; }
  const sensors_subfeature *getSub() const { return sub_; }
  double getVal() const { return val_; }
  std::string_view getName() const { return sub_->name; }

private:
  ContextPtrT               ctx_;
  const sensors_chip_name  *cname_;
  const sensors_feature    *feature_;
  int                       idx_;
  const sensors_subfeature *sub_;
  double                    val_;
};


class ChipT {
public:
  ChipT(ContextPtrT ctx, const sensors_chip_name *cname);
  std::string toString() const;
  static std::string toString(const sensors_chip_name *cname);

private:
  ContextPtrT              ctx_;
  const sensors_chip_name *cname_;
};


struct TupleT {
  std::string chip_;
  std::string label_;
  std::string name_;
  double      val_;
  bool        alarm_;
};


class AllIterT {
public:
  AllIterT(ContextPtrT ctx);
  AllIterT &operator++();
  explicit operator bool() const { return (bool) sIter_; }

  const sensors_chip_name *getChip() const { return cIter_.getChip(); }
  const sensors_feature *getFeature() const { return fIter_->getFeature(); }
  const sensors_subfeature *getSub() const { return sIter_->getSub(); }
  std::string_view getLabel() const { return fIter_->getLabel(); }
  double getVal() const { return sIter_->getVal(); }
  std::string_view getName() const { return sIter_->getName(); }

private:
  ContextPtrT                   ctx_;
  ChipIterT                     cIter_;
  std::unique_ptr<FeatureIterT> fIter_;
  std::unique_ptr<SubIterT>     sIter_;
  int                           cPos_;
  int                           fPos_;
  int                           sPos_;
};


class TupleIterT {
public:
  TupleIterT(ContextPtrT ctx);
  TupleIterT &operator++();
  explicit operator bool() const { return (bool) iter_; }

  TupleT getTuple() const;

private:
  AllIterT     iter_;
  std::string  key_;
  double       val_;
  bool         alarm_;
};

} // namespace zezax::sensorrd
