#pragma once

#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QString>

namespace chronexa::core {

class AppSettings;

} // namespace chronexa::core

namespace chronexa::activity {

class ActivityService;

class UserActivityController : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool trackingEnabled READ isTrackingEnabled WRITE
                 setTrackingEnabled NOTIFY trackingChanged)

  Q_PROPERTY(bool tracking READ isTracking NOTIFY trackingChanged)
  Q_PROPERTY(bool heldBySchedule READ isHeldBySchedule NOTIFY trackingChanged)
  Q_PROPERTY(QString scheduleHoldText READ scheduleHoldText NOTIFY
                 currentActivityChanged)

  Q_PROPERTY(bool idle READ isIdle NOTIFY currentActivityChanged)
  Q_PROPERTY(bool hasCurrent READ hasCurrent NOTIFY currentActivityChanged)
  Q_PROPERTY(
      QString currentAppName READ currentAppName NOTIFY currentActivityChanged)
  Q_PROPERTY(
      QString currentTitle READ currentTitle NOTIFY currentActivityChanged)
  Q_PROPERTY(
      qint64 currentSeconds READ currentSeconds NOTIFY currentActivityChanged)
  Q_PROPERTY(QString currentSecondsText READ currentSecondsText NOTIFY
                 currentActivityChanged)

public:
  UserActivityController(ActivityService *service, core::AppSettings *settings,
                         QObject *parent = nullptr);

  bool isTrackingEnabled() const;
  void setTrackingEnabled(bool enabled);
  bool isTracking() const;
  bool isHeldBySchedule() const;

  QString scheduleHoldText() const;

  bool isIdle() const;
  bool hasCurrent() const;
  QString currentAppName() const;
  QString currentTitle() const;
  qint64 currentSeconds() const;
  QString currentSecondsText() const;

  Q_INVOKABLE void toggleTracking();
  Q_INVOKABLE void clearActivities();

  Q_INVOKABLE QString formatDuration(qint64 seconds,
                                     bool compact = false) const;
  Q_INVOKABLE QString formatClock(const QDateTime &moment) const;
  Q_INVOKABLE QString formatDayLabel(const QDate &date) const;

signals:
  void trackingChanged();
  void currentActivityChanged();
  void historyChanged();

private:
  ActivityService *_service;
  core::AppSettings *_settings;
};

} // namespace chronexa::activity
