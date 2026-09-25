#include "ActivityQueryService.hpp"

#include <algorithm>

namespace {

constexpr int kMaxBuckets = 2048;

} // namespace

namespace chronexa::activity {

ActivityQueryService::ActivityQueryService(IActivityRepository &repository)
    : _repository(repository) {}

QList<Activity> ActivityQueryService::sessions(const QDateTime &from,
                                               const QDateTime &to,
                                               int limit) const {
  return joinContiguous(_repository.sessions(from, to, limit));
}

QList<AppTotal> ActivityQueryService::appTotals(const QDateTime &from,
                                                const QDateTime &to) const {
  return _repository.appTotals(from, to);
}

RangeStats ActivityQueryService::stats(const QDateTime &from,
                                       const QDateTime &to) const {
  return _repository.stats(from, to);
}

QPair<QDateTime, QDateTime> ActivityQueryService::bounds() const {
  return _repository.bounds();
}

QStringList ActivityQueryService::rankedAppNames(int limit) const {
  return _repository.rankedAppNames(limit);
}

void ActivityQueryService::setCategoryRules(CategoryRules rules) {
  _categoryRules = std::move(rules);
}

const CategoryRules &ActivityQueryService::categoryRules() const {
  return _categoryRules;
}

QString ActivityQueryService::categoryOf(const Activity &session) const {
  if (session.category) {
    return *session.category;
  }
  return _categoryRules.categorize(session.appName, session.title);
}

QList<CategoryTotal>
ActivityQueryService::categoryTotals(const QDateTime &from,
                                     const QDateTime &to) const {
  if (_categoryRules.isEmpty()) {
    return {};
  }
  return activity::categoryTotals(_repository.titleTotals(from, to),
                                  _categoryRules);
}

QList<BucketTotal>
ActivityQueryService::buckets(const QDateTime &from, const QDateTime &to,
                              Granularity granularity) const {
  QList<BucketTotal> buckets;
  if (!from.isValid() || !to.isValid() || from >= to) {
    return buckets;
  }

  for (QDateTime edge = alignToGranularity(from, granularity); edge < to;) {
    const QDateTime next = nextBucket(edge, granularity);
    if (next <= edge || buckets.size() >= kMaxBuckets) {
      break;
    }
    buckets.append({edge, next, 0});
    edge = next;
  }

  if (buckets.isEmpty()) {
    return buckets;
  }

  QList<qint64> edges;
  edges.reserve(buckets.size() + 1);
  for (const BucketTotal &bucket : buckets) {
    edges.append(bucket.start.toMSecsSinceEpoch());
  }
  edges.append(buckets.last().end.toMSecsSinceEpoch());

  // Accumulated in milliseconds and rounded once: rounding every overlap would
  // drop up to a second per session from the bucket it lands in.
  QList<qint64> milliseconds(buckets.size(), 0);

  for (const auto &interval : _repository.intervals(from, to)) {
    const qint64 start = interval.first;
    const qint64 end = interval.second;
    if (end <= start) {
      continue;
    }

    auto edge = std::upper_bound(edges.cbegin(), edges.cend(), start);
    int index = static_cast<int>(std::distance(edges.cbegin(), edge)) - 1;
    index = qMax(0, index);

    for (; index < buckets.size() && edges.at(index) < end; ++index) {
      const qint64 overlap =
          qMin(end, edges.at(index + 1)) - qMax(start, edges.at(index));
      if (overlap > 0) {
        milliseconds[index] += overlap;
      }
    }
  }

  for (int i = 0; i < buckets.size(); ++i) {
    buckets[i].seconds = milliseconds.at(i) / 1000;
  }

  return buckets;
}

} // namespace chronexa::activity
