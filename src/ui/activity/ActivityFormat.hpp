#pragma once

#include "domain/activity/ActivityStats.hpp"
#include "domain/activity/DailyGoals.hpp"
#include "domain/activity/GoalDigest.hpp"

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

QString goalKind(GoalKind kind);
QString goalAmount(const GoalProgress &progress);
QString goalStatus(const GoalProgress &progress);
QString goalAlertTitle(const GoalProgress &progress);
QString goalAlertMessage(const GoalProgress &progress);
QString goalLine(const DailyGoal &goal);
QString digestTitle(const GoalDigest &digest);
QString digestMessage(const GoalDigest &digest);
QString nextDigest(const QDateTime &due, DigestTime when);

} // namespace chronexa::activity::format
