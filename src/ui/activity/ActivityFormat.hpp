#pragma once

#include "domain/activity/ActivityStats.hpp"

#include <QDate>
#include <QDateTime>
#include <QString>
#include <QStringList>

namespace chronexa::activity::format {

QString duration(qint64 seconds, bool compact = false);

QString stopwatch(qint64 seconds);

QString clock(const QDateTime &moment);

QString dayLabel(const QDate &date);

QString bucketLabel(const BucketTotal &bucket, Granularity granularity);

QString bucketDescription(const BucketTotal &bucket, Granularity granularity);

QString rangeLabel(const QDateTime &from, const QDateTime &to);

QStringList splitApps(const QString &text);
QString joinApps(const QStringList &apps);

} // namespace chronexa::activity::format
