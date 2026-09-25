#pragma once

#include "domain/activity/Activity.hpp"
#include "domain/activity/ActivityStats.hpp"
#include "domain/activity/CategoryRules.hpp"
#include "infrastructure/activity/IActivityRepository.hpp"

#include <QList>

namespace chronexa::activity {

class ActivityQueryService {
public:
  explicit ActivityQueryService(IActivityRepository &repository);

  [[nodiscard]] QList<Activity>
  sessions(const QDateTime &from, const QDateTime &to, int limit = -1) const;
  [[nodiscard]] QList<AppTotal> appTotals(const QDateTime &from,
                                          const QDateTime &to) const;
  [[nodiscard]] RangeStats stats(const QDateTime &from,
                                 const QDateTime &to) const;

  [[nodiscard]] QList<BucketTotal> buckets(const QDateTime &from,
                                           const QDateTime &to,
                                           Granularity granularity) const;

  [[nodiscard]] QPair<QDateTime, QDateTime> bounds() const;
  [[nodiscard]] QStringList rankedAppNames(int limit) const;

  void setCategoryRules(CategoryRules rules);
  [[nodiscard]] const CategoryRules &categoryRules() const;
  [[nodiscard]] QString categoryOf(const Activity &session) const;
  QList<CategoryTotal> categoryTotals(const QDateTime &from,
                                      const QDateTime &to) const;

private:
  IActivityRepository &_repository;
  CategoryRules _categoryRules;
};

} // namespace chronexa::activity
