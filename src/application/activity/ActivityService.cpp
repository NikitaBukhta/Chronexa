#include "ActivityService.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QTimer>
#include <QtSystemDetection>

#include "infrastructure/activity/PollingActivityProvider.hpp"

#ifdef Q_OS_WIN
#include "infrastructure/activity/WindowsForegroundProbe.hpp"
using OSSpecificProbe = chronexa::activity::WindowsForegroundProbe;
#else
#error "Unsupported platform"
#endif

namespace {

Q_LOGGING_CATEGORY(lcActivity, "chronexa.activity.service")

} // namespace

namespace chronexa::activity {

namespace {

constexpr int kFlushIntervalMinutes = 1;

constexpr int kHeartbeatIntervalMs = 1000;

// Ceiling for sessions held back after a failed insert. Reached only when the
// repository stays unavailable for a long time; past it the oldest are dropped
// so an unwritable database cannot grow the process without bound.
constexpr int kMaxPendingSessions = 10000;

qint64 msUntilNextBoundary() {
  const QDateTime now = QDateTime::currentDateTime();
  const QTime time = now.time();
  const int nextMinute =
      ((time.minute() / kFlushIntervalMinutes) + 1) * kFlushIntervalMinutes;
  QDateTime next = now;
  next.setTime(QTime(time.hour(), 0));
  next = next.addSecs(nextMinute * 60);
  return now.msecsTo(next);
}

} // namespace

ActivityService::ActivityService(IActivityRepository &repository,
                                 QObject *parent)
    : ActivityService(repository,
                      std::make_unique<PollingActivityProvider>(
                          std::make_unique<OSSpecificProbe>()),
                      parent) {}

ActivityService::ActivityService(
    IActivityRepository &repository,
    std::unique_ptr<IUserActivityProvider> provider, QObject *parent)
    : QObject(parent), _repository(repository),
      _activityProvider(std::move(provider)) {
  applyTrackingState();

  scheduleNextFlush();

  auto *heartbeat = new QTimer(this);
  heartbeat->setInterval(kHeartbeatIntervalMs);
  heartbeat->setTimerType(Qt::CoarseTimer);
  connect(heartbeat, &QTimer::timeout, this, &ActivityService::onHeartbeat);
  heartbeat->start();

  qCInfo(lcActivity) << "ActivityService started, flush interval ="
                     << kFlushIntervalMinutes << "min";
}

ActivityService::~ActivityService() {
  if (_activityProvider) {
    _activityProvider->stop();
    flush(Notify::No);
  }
  if (!_pending.isEmpty()) {
    qCCritical(lcActivity) << "Shutting down with" << _pending.size()
                           << "unwritten sessions -- they are lost";
  }
  qCInfo(lcActivity) << "ActivityService stopped";
}

void ActivityService::setTrackingEnabled(bool enabled) {
  if (_trackingEnabled == enabled) {
    return;
  }
  _trackingEnabled = enabled;
  qCInfo(lcActivity) << "Tracking switch" << (enabled ? "on" : "off");
  applyTrackingState();
}

bool ActivityService::isTrackingEnabled() const { return _trackingEnabled; }

void ActivityService::setSchedule(const TrackingSchedule &schedule) {
  if (_schedule == schedule) {
    return;
  }
  _schedule = schedule;
  qCInfo(lcActivity) << "Schedule updated: enabled =" << schedule.enabled
                     << "window =" << schedule.start.toString(Qt::ISODate)
                     << "-" << schedule.end.toString(Qt::ISODate)
                     << "days =" << schedule.days;
  applyTrackingState();
}

TrackingSchedule ActivityService::schedule() const { return _schedule; }

void ActivityService::setCategoryRules(const QList<CategoryRule> &rules) {
  if (!_activityProvider) {
    return;
  }
  // The key runs on the provider's polling path. It gets rules compiled just
  // for it, so no QRegularExpression is shared with this thread.
  _activityProvider->setSessionKey(
      [compiled = CategoryRules(rules)](const QString &appName,
                                        const QString &title) {
        return compiled.categorize(appName, title);
      });
}

void ActivityService::setPrivacyRules(const QList<PrivacyRule> &rules) {
  _privacy = PrivacyRules(rules);
  qCInfo(lcActivity) << "Privacy rules updated:" << rules.size();
  redactPending();
  if (!_activityProvider) {
    return;
  }
  // As for the session key: a copy compiled for the polling path alone.
  _activityProvider->setPrivacyFilter(
      [compiled = PrivacyRules(rules)](const QString &appName,
                                       const QString &title) {
        return compiled.classify(appName, title);
      });
}

int ActivityService::applyPrivacyToHistory() {
  // Written first, so what was sampled up to now is covered too. Announced
  // like any flush: the rows it writes are new to the views either way.
  flush(Notify::Yes);

  const std::optional<QList<WindowRef>> windows = _repository.windows();
  if (!windows) {
    qCWarning(lcActivity) << "History unreadable -- privacy rules not applied";
    return -1;
  }

  QList<WindowRef> remove;
  QList<WindowRef> hideTitle;
  for (const WindowRef &window : *windows) {
    switch (_privacy.classify(window.appName, window.title)) {
    case Privacy::Exclude:
      remove.append(window);
      break;
    case Privacy::HideTitle:
      hideTitle.append(window);
      break;
    case Privacy::Record:
      break;
    }
  }

  const int changed = _repository.redact(remove, hideTitle);
  if (changed < 0) {
    qCWarning(lcActivity) << "Privacy rules could not be applied to history";
    return changed;
  }

  qCInfo(lcActivity) << "Privacy rules applied to history:" << remove.size()
                     << "windows removed," << hideTitle.size()
                     << "titles hidden," << changed << "sessions changed";
  if (changed > 0) {
    emit activityRecorded();
  }
  return changed;
}

void ActivityService::redactPending() {
  if (_privacy.isEmpty() || _pending.isEmpty()) {
    return;
  }

  QList<Activity> kept;
  kept.reserve(_pending.size());
  for (Activity &activity : _pending) {
    switch (_privacy.classify(activity.appName, activity.title)) {
    case Privacy::Exclude:
      continue;
    case Privacy::HideTitle:
      activity.title.clear();
      break;
    case Privacy::Record:
      break;
    }
    kept.append(std::move(activity));
  }
  _pending.swap(kept);
}

bool ActivityService::isTracking() const {
  return _activityProvider && _activityProvider->isRunning();
}

bool ActivityService::isHeldBySchedule() const {
  return _trackingEnabled && !_schedule.allows(QDateTime::currentDateTime());
}

QDateTime ActivityService::nextScheduleChange() const {
  return _schedule.nextChange(QDateTime::currentDateTime());
}

void ActivityService::applyTrackingState() {
  if (!_activityProvider) {
    return;
  }

  const bool shouldRun =
      _trackingEnabled && _schedule.allows(QDateTime::currentDateTime());
  if (shouldRun == _activityProvider->isRunning()) {
    return;
  }

  if (shouldRun) {
    _activityProvider->start();
  } else {
    _activityProvider->stop();
    flush(Notify::Yes);
  }

  emit trackingChanged();
  emit currentActivityChanged();
}

std::optional<Activity> ActivityService::currentSession() const {
  if (!_activityProvider) {
    return std::nullopt;
  }
  return _activityProvider->currentSession();
}

bool ActivityService::isIdle() const {
  return _activityProvider && _activityProvider->isIdle();
}

void ActivityService::requestClear() {
  if (_activityProvider) {
    _activityProvider->drainEvents();
  }
  // Held-back sessions predate the clear, so they must not be written after it.
  _pending.clear();
  _repository.clearAll();

  qCInfo(lcActivity) << "History cleared";
  emit cleared();
  emit activityRecorded();
}

void ActivityService::scheduleNextFlush() {
  const qint64 delayMs = msUntilNextBoundary();
  qCDebug(lcActivity) << "Next flush in" << delayMs << "ms";
  QTimer::singleShot(delayMs, this, &ActivityService::onFlushTick);
}

void ActivityService::onFlushTick() {
  flush(Notify::Yes);
  scheduleNextFlush();
}

void ActivityService::onHeartbeat() {
  // Re-checked here rather than from a timer armed at the next window edge: a
  // machine that sleeps through that edge would never fire it.
  if (_schedule.enabled) {
    applyTrackingState();
  }
  emit currentActivityChanged();
}

void ActivityService::flush(Notify notify) {
  if (!_activityProvider) {
    return;
  }

  // Draining the provider is destructive, so anything the repository refuses
  // has to be kept here: AppInitializer deliberately carries on when the
  // database cannot be opened, and dropping the batch on failure discarded a
  // full minute of tracked time every minute with only a log line to show.
  _pending += _activityProvider->drainEvents();
  redactPending();
  if (_pending.isEmpty()) {
    return;
  }

  qCInfo(lcActivity) << "Flushing" << _pending.size() << "sessions at"
                     << QDateTime::currentDateTime().toString(Qt::ISODate);

  if (!_repository.insertBatch(_pending)) {
    if (_pending.size() > kMaxPendingSessions) {
      const qsizetype dropped = _pending.size() - kMaxPendingSessions;
      _pending.remove(0, dropped);
      qCCritical(lcActivity) << "Insert failed and the backlog is full: dropped"
                             << dropped << "oldest sessions";
    } else {
      qCWarning(lcActivity) << "Insert failed, keeping" << _pending.size()
                            << "sessions for the next flush";
    }
    return;
  }

  _pending.clear();
  if (notify == Notify::Yes) {
    emit activityRecorded();
  }
}

} // namespace chronexa::activity
