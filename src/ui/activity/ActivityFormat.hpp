#pragma once

#include "domain/activity/ActivityStats.hpp"

#include <QDate>
#include <QDateTime>
#include <QString>

namespace chronexa::activity::format {

QString duration(qint64 seconds, bool compact = false);

QString stopwatch(qint64 seconds);

QString clock(const QDateTime &moment);

QString dayLabel(const QDate &date);

QString bucketLabel(const BucketTotal &bucket, Granularity granularity);

QString bucketDescription(const BucketTotal &bucket, Granularity granularity);

QString rangeLabel(const QDateTime &from, const QDateTime &to);

} // namespace chronexa::activity::format
