#include "PollingActivityProvider.hpp"

#include "domain/activity/PrivacyRules.hpp"

#include <QLoggingCategory>
#include <QMutexLocker>
#include <QTimer>

namespace {

Q_LOGGING_CATEGORY(lcProvider, "chronexa.activity.provider")

} // namespace

namespace chronexa::activity {

PollingActivityProvider::PollingActivityProvider(
    std::unique_ptr<IForegroundProbe> probe, Clock clock)
    : _probe(std::move(probe)), _clock(std::move(clock)) {
  if (!_clock) {
    _clock = []() { return QDateTime::currentDateTime(); };
  }
}

PollingActivityProvider::~PollingActivityProvider() { stop(); }

void PollingActivityProvider::start() {
  if (_running) {
    return;
  }

  if (!_timer) {
    _timer = std::make_unique<QTimer>();
    _timer->setTimerType(Qt::CoarseTimer);
    _timer->setInterval(kPollIntervalMs);
    QObject::connect(_timer.get(), &QTimer::timeout, _timer.get(),
                     [this]() { poll(); });
  }

  _running = true;
  _timer->start();
  qCInfo(lcProvider) << "Foreground sampling started every" << kPollIntervalMs
                     << "ms";
}

void PollingActivityProvider::stop() {
  if (!_running) {
    return;
  }
  _running = false;

  if (_timer) {
    _timer->stop();
  }

  QMutexLocker locker(&_mutex);
  closeCurrentLocked(_clock());
  qCInfo(lcProvider) << "Foreground sampling stopped";
}

bool PollingActivityProvider::isRunning() const { return _running; }

bool PollingActivityProvider::isIdle() const {
  QMutexLocker locker(&_mutex);
  return _idle;
}

void PollingActivityProvider::setSessionKey(SessionKey key) {
  QMutexLocker locker(&_mutex);
  _sessionKey = std::move(key);
}

void PollingActivityProvider::setPrivacyFilter(PrivacyFilter filter) {
  QMutexLocker locker(&_mutex);
  _privacyFilter = std::move(filter);
}

void PollingActivityProvider::poll() {
  const QDateTime now = _clock();
  // Sampled outside the lock: reading another process can block.
  const ForegroundSample sample = _probe->sample();

  QMutexLocker locker(&_mutex);

  if (sample.idle) {
    if (!_idle) {
      closeCurrentLocked(now);
      _idle = true;
      qCDebug(lcProvider) << "User went idle";
    }
    return;
  }
  _idle = false;

  if (sample.appName.isEmpty()) {
    closeCurrentLocked(now);
    return;
  }

  const Privacy privacy = _privacyFilter
                              ? _privacyFilter(sample.appName, sample.title)
                              : Privacy::Record;
  if (privacy == Privacy::Exclude) {
    closeCurrentLocked(now);
    return;
  }
  const bool hidden = privacy == Privacy::HideTitle;
  const QString title = hidden ? QString() : sample.title;

  // Keyed on the application and the session key, not on (app, title): a
  // media player, a terminal printing progress or a browser tab with a
  // countdown rewrites its title on every poll, and splitting the session there
  // produced nothing but 1-second fragments that closeCurrentLocked discarded
  // below the minimum -- so those apps recorded no time at all. The key is the
  // title's category, so switching from a work tab to YouTube does split,
  // while a ticking title stays one session. Otherwise the title is
  // descriptive, and follows the window it belongs to -- except that a hidden
  // title splits too, or the private window's time would be booked under the
  // visible title next to it.
  const QString key =
      _sessionKey ? _sessionKey(sample.appName, title) : QString();
  if (_hasCurrent && _current.appName == sample.appName && _currentKey == key &&
      _currentHidden == hidden) {
    _current.title = title;
    _current.endedOn = now;
    return;
  }

  closeCurrentLocked(now);
  openCurrentLocked(sample.appName, title, key, hidden, now);
}

void PollingActivityProvider::closeCurrentLocked(const QDateTime &at) {
  if (!_hasCurrent) {
    return;
  }

  _current.endedOn = at.isValid() ? at : _current.endedOn;
  if (_current.durationSeconds() >= kMinSessionSeconds) {
    _buffer.append(_current);
  }

  _current = Activity();
  _currentKey.clear();
  _currentHidden = false;
  _currentSince = QDateTime();
  _hasCurrent = false;
}

void PollingActivityProvider::openCurrentLocked(const QString &appName,
                                                const QString &title,
                                                const QString &key, bool hidden,
                                                const QDateTime &at) {
  _currentKey = key;
  _currentHidden = hidden;
  _current.appName = appName;
  _current.title = title;
  _current.startedOn = at;
  _current.endedOn = at;
  _currentSince = at;
  _hasCurrent = true;
}

QList<Activity> PollingActivityProvider::drainEvents() {
  QMutexLocker locker(&_mutex);

  // Split the open session: hand over the part that already happened and let
  // the rest keep running, so draining neither loses nor double counts it.
  if (_hasCurrent && _current.durationSeconds() >= kMinSessionSeconds) {
    _buffer.append(_current);
    _current.startedOn = _current.endedOn;
  }

  QList<Activity> drained;
  drained.swap(_buffer);
  return drained;
}

std::optional<Activity> PollingActivityProvider::currentSession() const {
  QMutexLocker locker(&_mutex);
  if (!_hasCurrent || _idle) {
    return std::nullopt;
  }

  Activity session = _current;
  session.startedOn = _currentSince;
  return session;
}

} // namespace chronexa::activity
