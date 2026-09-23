#pragma once

#include "domain/activity/TrackingSchedule.hpp"

#include <QObject>
#include <QSettings>
#include <QString>

#include <memory>

namespace chronexa::core {

class AppSettings : public QObject {
  Q_OBJECT

  Q_PROPERTY(bool trackingEnabled READ trackingEnabled WRITE setTrackingEnabled
                 NOTIFY trackingEnabledChanged)
  Q_PROPERTY(bool scheduleEnabled READ scheduleEnabled WRITE setScheduleEnabled
                 NOTIFY scheduleChanged)
  Q_PROPERTY(int scheduleStartMinutes READ scheduleStartMinutes WRITE
                 setScheduleStartMinutes NOTIFY scheduleChanged)
  Q_PROPERTY(int scheduleEndMinutes READ scheduleEndMinutes WRITE
                 setScheduleEndMinutes NOTIFY scheduleChanged)
  Q_PROPERTY(int scheduleDays READ scheduleDays WRITE setScheduleDays NOTIFY
                 scheduleChanged)
  Q_PROPERTY(
      QString language READ language WRITE setLanguage NOTIFY languageChanged)

  Q_PROPERTY(QString historyPath READ historyPath CONSTANT)

public:
  explicit AppSettings(QObject *parent = nullptr);
  ~AppSettings() override;

  bool trackingEnabled() const;
  void setTrackingEnabled(bool enabled);

  bool scheduleEnabled() const;
  void setScheduleEnabled(bool enabled);

  int scheduleStartMinutes() const;
  void setScheduleStartMinutes(int minutes);

  int scheduleEndMinutes() const;
  void setScheduleEndMinutes(int minutes);

  int scheduleDays() const;
  void setScheduleDays(int days);

  QString language() const;
  void setLanguage(const QString &language);

  QString historyPath() const;

  activity::TrackingSchedule schedule() const;

signals:
  void trackingEnabledChanged();
  void scheduleChanged();
  void languageChanged();

private:
  void store(const QString &key, const QVariant &value);

  std::unique_ptr<QSettings> _store;

  bool _trackingEnabled = true;
  activity::TrackingSchedule _schedule;
  QString _language;
};

} // namespace chronexa::core
