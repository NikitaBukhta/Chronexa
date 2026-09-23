#include "ActivityStats.hpp"

namespace chronexa::activity {

QString granularityKey(Granularity granularity) {
  switch (granularity) {
  case Granularity::Hour:
    return QStringLiteral("hour");
  case Granularity::Day:
    return QStringLiteral("day");
  case Granularity::Week:
    return QStringLiteral("week");
  case Granularity::Month:
    return QStringLiteral("month");
  }
  return QStringLiteral("day");
}

Granularity granularityFromKey(const QString &key) {
  if (key == QStringLiteral("hour")) {
    return Granularity::Hour;
  }
  if (key == QStringLiteral("week")) {
    return Granularity::Week;
  }
  if (key == QStringLiteral("month")) {
    return Granularity::Month;
  }
  return Granularity::Day;
}

Granularity suggestGranularity(const QDateTime &from, const QDateTime &to) {
  if (!from.isValid() || !to.isValid() || from >= to) {
    return Granularity::Day;
  }

  const qint64 days = from.daysTo(to);
  if (days <= 2) {
    return Granularity::Hour;
  }
  if (days <= 70) {
    return Granularity::Day;
  }
  if (days <= 400) {
    return Granularity::Week;
  }
  return Granularity::Month;
}

QDateTime alignToGranularity(const QDateTime &moment, Granularity granularity) {
  if (!moment.isValid()) {
    return moment;
  }

  const QDate date = moment.date();
  switch (granularity) {
  case Granularity::Hour:
    return QDateTime(date, QTime(moment.time().hour(), 0));
  case Granularity::Day:
    return date.startOfDay();
  case Granularity::Week:
    return date.addDays(-(date.dayOfWeek() - 1)).startOfDay();
  case Granularity::Month:
    return QDate(date.year(), date.month(), 1).startOfDay();
  }
  return moment;
}

QDateTime nextBucket(const QDateTime &bucketStart, Granularity granularity) {
  switch (granularity) {
  case Granularity::Hour:
    return bucketStart.addSecs(60 * 60);
  case Granularity::Day:
    return bucketStart.date().addDays(1).startOfDay();
  case Granularity::Week:
    return bucketStart.date().addDays(7).startOfDay();
  case Granularity::Month:
    return bucketStart.date().addMonths(1).startOfDay();
  }
  return bucketStart;
}

} // namespace chronexa::activity
