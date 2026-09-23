#include "ActivityFormat.hpp"

#include <QLocale>
#include <QObject>

namespace chronexa::activity::format {

QString duration(qint64 seconds, bool compact) {
  // The unit suffixes are words, not symbols: "17s" reads as English in the
  // Russian and Ukrainian UI, so they go through the catalogue like any text.
  if (seconds <= 0) {
    return QObject::tr("0m");
  }

  const qint64 hours = seconds / 3600;
  const qint64 minutes = (seconds % 3600) / 60;
  const qint64 rest = seconds % 60;

  if (hours > 0) {
    return minutes > 0 ? QObject::tr("%1h %2m").arg(hours).arg(minutes)
                       : QObject::tr("%1h").arg(hours);
  }
  if (minutes > 0) {
    if (compact || rest == 0) {
      return QObject::tr("%1m").arg(minutes);
    }
    return QObject::tr("%1m %2s").arg(minutes).arg(rest);
  }
  return QObject::tr("%1s").arg(rest);
}

QString stopwatch(qint64 seconds) {
  seconds = qMax<qint64>(0, seconds);
  const qint64 hours = seconds / 3600;
  const qint64 minutes = (seconds % 3600) / 60;
  const qint64 rest = seconds % 60;

  if (hours > 0) {
    return QStringLiteral("%1:%2:%3")
        .arg(hours)
        .arg(minutes, 2, 10, QLatin1Char('0'))
        .arg(rest, 2, 10, QLatin1Char('0'));
  }
  return QStringLiteral("%1:%2").arg(minutes).arg(rest, 2, 10,
                                                  QLatin1Char('0'));
}

QString clock(const QDateTime &moment) {
  if (!moment.isValid()) {
    return QStringLiteral("--:--");
  }
  return QLocale().toString(moment.time(), QLocale::ShortFormat);
}

QString dayLabel(const QDate &date) {
  if (!date.isValid()) {
    return QString();
  }

  const QDate today = QDate::currentDate();
  if (date == today) {
    return QObject::tr("Today");
  }
  if (date == today.addDays(-1)) {
    return QObject::tr("Yesterday");
  }

  const QLocale locale = QLocale();
  const QString weekday =
      locale.dayName(date.dayOfWeek(), QLocale::ShortFormat);
  const QString month = locale.monthName(date.month(), QLocale::ShortFormat);
  if (date.year() == today.year()) {
    return QStringLiteral("%1, %2 %3").arg(weekday).arg(date.day()).arg(month);
  }
  return QStringLiteral("%1, %2 %3 %4")
      .arg(weekday)
      .arg(date.day())
      .arg(month)
      .arg(date.year());
}

QString bucketLabel(const BucketTotal &bucket, Granularity granularity) {
  if (!bucket.start.isValid()) {
    return QString();
  }

  const QLocale locale = QLocale();
  switch (granularity) {
  case Granularity::Hour:
    return bucket.start.time().toString(QStringLiteral("HH"));
  case Granularity::Day:
    return QStringLiteral("%1 %2")
        .arg(locale.dayName(bucket.start.date().dayOfWeek(),
                            QLocale::NarrowFormat))
        .arg(bucket.start.date().day());
  case Granularity::Week:
    return QStringLiteral("%1 %2")
        .arg(bucket.start.date().day())
        .arg(locale.monthName(bucket.start.date().month(),
                              QLocale::ShortFormat));
  case Granularity::Month:
    return locale.monthName(bucket.start.date().month(), QLocale::ShortFormat);
  }
  return QString();
}

QString bucketDescription(const BucketTotal &bucket, Granularity granularity) {
  if (!bucket.start.isValid()) {
    return QString();
  }

  switch (granularity) {
  case Granularity::Hour:
    return QStringLiteral("%1, %2 - %3")
        .arg(dayLabel(bucket.start.date()))
        .arg(clock(bucket.start))
        .arg(clock(bucket.end));
  case Granularity::Day:
    return dayLabel(bucket.start.date());
  case Granularity::Week:
    return QObject::tr("Week of %1").arg(dayLabel(bucket.start.date()));
  case Granularity::Month:
    return QStringLiteral("%1 %2")
        .arg(QLocale().monthName(bucket.start.date().month()))
        .arg(bucket.start.date().year());
  }
  return QString();
}

QString rangeLabel(const QDateTime &from, const QDateTime &to) {
  if (!from.isValid() || !to.isValid()) {
    return QString();
  }

  const QLocale locale = QLocale();
  const QDate firstDay = from.date();
  const QDate lastDay =
      to.time() == QTime(0, 0) ? to.date().addDays(-1) : to.date();

  if (firstDay == lastDay) {
    return dayLabel(firstDay);
  }

  const bool wholeMonth = firstDay.day() == 1 &&
                          lastDay == QDate(lastDay.year(), lastDay.month(),
                                           lastDay.daysInMonth()) &&
                          firstDay.month() == lastDay.month() &&
                          firstDay.year() == lastDay.year();
  if (wholeMonth) {
    return QStringLiteral("%1 %2")
        .arg(locale.monthName(firstDay.month()))
        .arg(firstDay.year());
  }

  const QString first =
      firstDay.year() == lastDay.year()
          ? QStringLiteral("%1 %2")
                .arg(firstDay.day())
                .arg(locale.monthName(firstDay.month(), QLocale::ShortFormat))
          : QStringLiteral("%1 %2 %3")
                .arg(firstDay.day())
                .arg(locale.monthName(firstDay.month(), QLocale::ShortFormat))
                .arg(firstDay.year());

  return QStringLiteral("%1 - %2 %3 %4")
      .arg(first)
      .arg(lastDay.day())
      .arg(locale.monthName(lastDay.month(), QLocale::ShortFormat))
      .arg(lastDay.year());
}

} // namespace chronexa::activity::format
