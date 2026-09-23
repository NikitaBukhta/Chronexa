#pragma once

#include "domain/activity/Activity.hpp"
#include "domain/activity/ActivityStats.hpp"

#include <QList>
#include <QPair>
#include <QStringList>

namespace chronexa::activity {

class IActivityRepository {
public:
  using Interval = QPair<qint64, qint64>;

  virtual ~IActivityRepository() = default;

  virtual bool open() = 0;
  virtual bool insertBatch(const QList<Activity> &activities) = 0;
  virtual bool clearAll() = 0;

  virtual QList<Activity> sessions(const QDateTime &from, const QDateTime &to,
                                   int limit = -1) const = 0;

  virtual QList<AppTotal> appTotals(const QDateTime &from,
                                    const QDateTime &to) const = 0;

  virtual QList<Interval> intervals(const QDateTime &from,
                                    const QDateTime &to) const = 0;

  virtual RangeStats stats(const QDateTime &from,
                           const QDateTime &to) const = 0;

  virtual QPair<QDateTime, QDateTime> bounds() const = 0;
  virtual QStringList rankedAppNames(int limit) const = 0;
};

} // namespace chronexa::activity
