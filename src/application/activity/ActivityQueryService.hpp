#pragma once

#include "domain/activity/Activity.hpp"
#include "domain/activity/ActivityStats.hpp"
#include "infrastructure/activity/IActivityRepository.hpp"

#include <QList>
#include <QPair>
#include <QStringList>

namespace chronexa::activity {

class ActivityQueryService {
public:
  explicit ActivityQueryService(IActivityRepository &repository);

  QList<Activity> sessions(const QDateTime &from, const QDateTime &to,
                           int limit = -1) const;
  QList<AppTotal> appTotals(const QDateTime &from, const QDateTime &to) const;
  RangeStats stats(const QDateTime &from, const QDateTime &to) const;

  QList<BucketTotal> buckets(const QDateTime &from, const QDateTime &to,
                             Granularity granularity) const;

  QPair<QDateTime, QDateTime> bounds() const;
  QStringList rankedAppNames(int limit) const;

private:
  IActivityRepository &_repository;
};

} // namespace chronexa::activity
