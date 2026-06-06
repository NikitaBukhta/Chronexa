#include "ActivityService.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QTimer>
#include <QtSystemDetection>

#ifdef Q_OS_WIN
#include "infrastructure/activity/WindowsActivityProvider.hpp"
using OSSpecificProvider = chronexa::activity::WindowsActivityProvider;
#else
#error "Unsupported platform"
#endif

namespace {

Q_LOGGING_CATEGORY(lcActivity, "chronexa.activity.service")

}

namespace chronexa::activity {

namespace {

constexpr int kTickIntervalMinutes = 5;

qint64 msUntilNextBoundary() {
  const QDateTime now = QDateTime::currentDateTime();
  const QTime t = now.time();
  const int nextMinute =
      ((t.minute() / kTickIntervalMinutes) + 1) * kTickIntervalMinutes;
  QDateTime next = now;
  next.setTime(QTime(t.hour(), 0));
  next = next.addSecs(nextMinute * 60);
  return now.msecsTo(next);
}

} // namespace

ActivityService::ActivityService(QObject *parent) : QObject(parent) {
  _activityProvider = std::make_unique<OSSpecificProvider>();
  _activityProvider->start();
  scheduleNextTick();
  qCInfo(lcActivity) << "ActivityService started, tick interval ="
                     << kTickIntervalMinutes << "min";
}

ActivityService::~ActivityService() {
  if (_activityProvider) {
    _activityProvider->stop();
  }
  qCInfo(lcActivity) << "ActivityService stopped";
}

void ActivityService::requestClear() {
  if (_activityProvider) {
    _activityProvider->drainEvents();
  }

  qCInfo(lcActivity) << "Clear requested";
  emit cleared();
}

void ActivityService::scheduleNextTick() {
  const qint64 delayMs = msUntilNextBoundary();
  qCDebug(lcActivity) << "Next tick in" << delayMs << "ms";
  QTimer::singleShot(delayMs, this, &ActivityService::onTick);
}

void ActivityService::onTick() {
  QList<Activity> activities = _activityProvider->drainEvents();
  qCInfo(lcActivity) << "Tick at"
                     << QDateTime::currentDateTime().toString(Qt::ISODate)
                     << "-- drained" << activities.size() << "activities";
  emit activityUpdated(std::move(activities));
  scheduleNextTick();
}

} // namespace chronexa::activity
