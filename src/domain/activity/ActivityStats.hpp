#pragma once

#include <QDateTime>
#include <QString>

#include <optional>

namespace chronexa::activity {

enum class Granularity { Hour, Day, Week, Month };

QString granularityKey(Granularity granularity);
Granularity granularityFromKey(const QString &key);

Granularity suggestGranularity(const QDateTime &from, const QDateTime &to);

QDateTime alignToGranularity(const QDateTime &moment, Granularity granularity);

QDateTime nextBucket(const QDateTime &bucketStart, Granularity granularity);

struct AppTotal {
  QString appName;
  qint64 seconds = 0;
  int sessionCount = 0;
};

struct TitleTotal {
  QString appName;
  QString title;
  qint64 milliseconds = 0;
  int sessionCount = 0;
  std::optional<QString> category;
};

struct BucketTotal {
  QDateTime start;
  QDateTime end;
  qint64 seconds = 0;
};

struct RangeStats {
  qint64 totalSeconds = 0;
  int sessionCount = 0;
  int appCount = 0;
  qint64 longestSessionSeconds = 0;
  QString longestSessionApp;
  QDateTime firstActivity;
  QDateTime lastActivity;
};

} // namespace chronexa::activity
