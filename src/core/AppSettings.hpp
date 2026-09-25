#pragma once

#include "domain/activity/CategoryRules.hpp"
#include "domain/activity/PrivacyRules.hpp"
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

  // False until rules were saved once, so first-run defaults can be told
  // apart from a list the user emptied on purpose.
  bool hasCategoryRules() const;
  const QList<activity::CategoryRule> &categoryRules() const;
  void setCategoryRules(const QList<activity::CategoryRule> &rules);

  // As for categories: false until saved once.
  bool hasPrivacyRules() const;
  const QList<activity::PrivacyRule> &privacyRules() const;
  void setPrivacyRules(const QList<activity::PrivacyRule> &rules);

signals:
  void trackingEnabledChanged();
  void scheduleChanged();
  void languageChanged();
  void categoryRulesChanged();
  void privacyRulesChanged();

private:
  void store(const QString &key, const QVariant &value);

  std::unique_ptr<QSettings> _store;

  bool _trackingEnabled = true;
  activity::TrackingSchedule _schedule;
  QString _language;
  QList<activity::CategoryRule> _categoryRules;
  QList<activity::PrivacyRule> _privacyRules;
};

} // namespace chronexa::core
