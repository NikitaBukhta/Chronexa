#pragma once

#include "IUserActivityProvider.hpp"

#include <QHash>
#include <QList>
#include <QMutex>
#include <QString>

#include <memory>

class QTimer;

namespace chronexa::activity {

class WindowsActivityProvider : public IUserActivityProvider {
public:
  WindowsActivityProvider();
  ~WindowsActivityProvider() override;

  void start() override;
  void stop() override;
  bool isRunning() const override;

  void setSessionKey(SessionKey key) override;

  QList<Activity> drainEvents() override;
  std::optional<Activity> currentSession() const override;
  bool isIdle() const override;

private:
  void poll();

  void closeCurrentLocked(const QDateTime &at);
  void openCurrentLocked(const QString &appName, const QString &title,
                         const QString &key, const QDateTime &at);

  QString appNameForWindow(void *windowHandle);

  std::unique_ptr<QTimer> _timer;

  mutable QMutex _mutex;
  QList<Activity> _buffer;

  SessionKey _sessionKey;

  Activity _current;
  QString _currentKey;
  QDateTime _currentSince;
  bool _hasCurrent = false;

  bool _idle = false;
  bool _running = false;

  QHash<QString, QString> _appNameCache;
};

} // namespace chronexa::activity
