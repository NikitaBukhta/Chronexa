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
enum class EditResult;

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

  [[nodiscard]] bool isTrackingEnabled() const;
  void setTrackingEnabled(bool enabled);
  [[nodiscard]] bool isTracking() const;
  [[nodiscard]] bool isHeldBySchedule() const;

  [[nodiscard]] QString scheduleHoldText() const;

  [[nodiscard]] bool isIdle() const;
  [[nodiscard]] bool hasCurrent() const;
  [[nodiscard]] QString currentAppName() const;
  [[nodiscard]] QString currentTitle() const;
  [[nodiscard]] qint64 currentSeconds() const;
  [[nodiscard]] QString currentSecondsText() const;

  Q_INVOKABLE void toggleTracking();
  Q_INVOKABLE void clearActivities();
  Q_INVOKABLE int applyPrivacyToHistory();
  Q_INVOKABLE QString editSession(const QString &appName, const QString &title,
                                  const QDateTime &from, const QDateTime &to,
                                  const QString &newAppName,
                                  const QString &newTitle,
                                  const QString &categoryMode,
                                  const QString &category);
  Q_INVOKABLE QString cutSession(const QString &appName, const QString &title,
                                 const QDateTime &from, const QDateTime &to,
                                 const QDateTime &cutFrom,
                                 const QDateTime &cutTo);
  Q_INVOKABLE [[nodiscard]] QString formatDuration(qint64 seconds,
                                                   bool compact = false) const;
  Q_INVOKABLE [[nodiscard]] QString formatClock(const QDateTime &moment) const;
  Q_INVOKABLE [[nodiscard]] QString formatDayLabel(const QDate &date) const;

signals:
  void trackingChanged();
  void currentActivityChanged();
  void historyChanged();

private:
  [[nodiscard]] QString editResultText(EditResult result) const;

private:
  ActivityService *_service;
  core::AppSettings *_settings;
};

} // namespace chronexa::activity
