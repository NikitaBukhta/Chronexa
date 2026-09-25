#pragma once

#include "IForegroundProbe.hpp"
#include "IUserActivityProvider.hpp"

#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QString>

#include <functional>
#include <memory>

class QTimer;

namespace chronexa::activity {

// Turns foreground samples into sessions. Platform-independent: the OS is
// behind the probe, and the clock is injectable, so every session rule here
// runs in a unit test.
class PollingActivityProvider : public IUserActivityProvider {
public:
  using Clock = std::function<QDateTime()>;

  static constexpr int kPollIntervalMs = 1000;
  static constexpr qint64 kMinSessionSeconds = 2;

  explicit PollingActivityProvider(std::unique_ptr<IForegroundProbe> probe,
                                   Clock clock = {});
  ~PollingActivityProvider() override;

  void start() override;
  void stop() override;
  bool isRunning() const override;

  void setSessionKey(SessionKey key) override;
  void setPrivacyFilter(PrivacyFilter filter) override;

  QList<Activity> drainEvents() override;
  std::optional<Activity> currentSession() const override;
  bool isIdle() const override;

  // One sampling step. Driven by the timer while running; public so tests can
  // step through a scripted sequence of samples.
  void poll();

private:
  void closeCurrentLocked(const QDateTime &at);
  void openCurrentLocked(const QString &appName, const QString &title,
                         const QString &key, bool hidden, const QDateTime &at);

  std::unique_ptr<IForegroundProbe> _probe;
  Clock _clock;
  std::unique_ptr<QTimer> _timer;

  mutable QMutex _mutex;
  QList<Activity> _buffer;

  SessionKey _sessionKey;
  PrivacyFilter _privacyFilter;

  Activity _current;
  QString _currentKey;
  // Whether the open session's title is withheld; a change splits it.
  bool _currentHidden = false;
  QDateTime _currentSince;
  bool _hasCurrent = false;

  bool _idle = false;
  bool _running = false;
};

} // namespace chronexa::activity
