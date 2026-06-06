#include "ActivityCollector.hpp"

#include <QMutexLocker>
#include <qloggingcategory.h>

Q_LOGGING_CATEGORY(lcActivityCollection, "chronexa.activity.collector")

namespace chronexa::activity {

namespace {

constexpr qint64 kMinActivityDurationMs = 1000;
constexpr qint64 kMergeGapMs = 2000;

bool isSameActivity(const Activity &lhs, const Activity &rhs) {
  return lhs.appId == rhs.appId && lhs.title == rhs.title && lhs.tabDomain == rhs.tabDomain;
}

} // namespace

void ActivityCollector::notifyActivityChanged(const QString &appId, const QString &appName, const QString &title,
                                              const QString &tabDomain, const QDateTime &when) {
  qCDebug(lcActivityCollection) << "appId: " << appId << " appName: " << appName << " title: " << title
                                << " tabDomain: " << tabDomain << " when: " << when;

  Activity next;
  next.appId = appId;
  next.appName = appName;
  next.title = title;
  next.tabDomain = tabDomain;
  next.startedOn = when;

  QMutexLocker locker(&_activityMutex);
  if (_currentActivity && isSameActivity(*_currentActivity, next)) {
    return;
  }

  closeCurrentLocked(when);
  _currentActivity = std::move(next);
}

QList<Activity> ActivityCollector::drainHandled(const QDateTime &now) {
  QMutexLocker locker(&_activityMutex);

  std::optional<Activity> reopened;
  if (_currentActivity) {
    // Re-open the same activity from `now` so the span between drain stays continuous.
    reopened = *_currentActivity;
    reopened->startedOn = now;
  }
  closeCurrentLocked(now);
  _currentActivity = std::move(reopened);

  return std::move(_activityList);
}

void ActivityCollector::reset() {
  QMutexLocker locker(&_activityMutex);
  _currentActivity.reset();
  _activityList.clear();
}

void ActivityCollector::closeCurrentLocked(const QDateTime &when) {
  if (!_currentActivity) {
    return;
  }

  Activity finished = std::move(*_currentActivity);
  _currentActivity.reset();
  finished.endedOn = when;

  const qint64 durationMs = finished.startedOn.msecsTo(finished.endedOn);
  if (durationMs < kMinActivityDurationMs) {
    return;
  }

  if (!_activityList.isEmpty()) {
    Activity &last = _activityList.last();
    if (isSameActivity(last, finished) && last.endedOn.msecsTo(finished.startedOn) <= kMergeGapMs) {
      last.endedOn = finished.endedOn;
      return;
    }
  }
  _activityList.append(std::move(finished));
}

} // namespace chronexa::activity
