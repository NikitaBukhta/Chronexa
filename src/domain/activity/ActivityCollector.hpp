#pragma once

#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QString>
#include <optional>

#include "domain/activity/Activity.hpp"

namespace chronexa::activity {

class ActivityCollector {
public:
  void notifyActivityChanged(const QString &appId, const QString &appName, const QString &title,
                             const QString &tabDomain, const QDateTime &when);
  QList<Activity> drainHandled(const QDateTime &now);
  void reset();

private:
  void closeCurrentLocked(const QDateTime &when);

private:
  std::optional<Activity> _currentActivity;
  QList<Activity> _activityList;
  QMutex _activityMutex;
};

} // namespace chronexa::activity
