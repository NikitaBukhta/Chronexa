#pragma once

#include "domain/activity/CategoryRules.hpp"
#include "domain/activity/DailyGoals.hpp"
#include "domain/activity/GoalDigest.hpp"
#include "domain/activity/PrivacyRules.hpp"
#include "domain/activity/TrackingSchedule.hpp"

#include <QDate>
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
  Q_PROPERTY(bool goalNotifications READ goalNotifications WRITE
                 setGoalNotifications NOTIFY goalNotificationsChanged)
  // "workstart", "custom" or "off".
  Q_PROPERTY(QString goalDigestTime READ goalDigestTime WRITE setGoalDigestTime
                 NOTIFY goalDigestChanged)
  Q_PROPERTY(int goalDigestMinutes READ goalDigestMinutes WRITE
                 setGoalDigestMinutes NOTIFY goalDigestChanged)
  Q_PROPERTY(bool goalDigestCatchUp READ goalDigestCatchUp WRITE
                 setGoalDigestCatchUp NOTIFY goalDigestChanged)

  Q_PROPERTY(QString historyPath READ historyPath CONSTANT)

public:
  explicit AppSettings(QObject *parent = nullptr);
  ~AppSettings() override;

  [[nodiscard]] bool trackingEnabled() const;
  void setTrackingEnabled(bool enabled);

  [[nodiscard]] bool scheduleEnabled() const;
  void setScheduleEnabled(bool enabled);

  [[nodiscard]] int scheduleStartMinutes() const;
  void setScheduleStartMinutes(int minutes);

  [[nodiscard]] int scheduleEndMinutes() const;
  void setScheduleEndMinutes(int minutes);

  [[nodiscard]] int scheduleDays() const;
  void setScheduleDays(int days);

  [[nodiscard]] QString language() const;
  void setLanguage(const QString &language);

  [[nodiscard]] QString historyPath() const;

  [[nodiscard]] activity::TrackingSchedule schedule() const;

  // False until rules were saved once, so first-run defaults can be told
  // apart from a list the user emptied on purpose.
  [[nodiscard]] bool hasCategoryRules() const;
  [[nodiscard]] const QList<activity::CategoryRule> &categoryRules() const;
  void setCategoryRules(const QList<activity::CategoryRule> &rules);

  // As for categories: false until saved once.
  [[nodiscard]] bool hasPrivacyRules() const;
  [[nodiscard]] const QList<activity::PrivacyRule> &privacyRules() const;
  void setPrivacyRules(const QList<activity::PrivacyRule> &rules);

  // As for categories: false until saved once.
  [[nodiscard]] bool hasDailyGoals() const;
  [[nodiscard]] const QList<activity::DailyGoal> &dailyGoals() const;
  void setDailyGoals(const QList<activity::DailyGoal> &goals);

  [[nodiscard]] bool goalNotifications() const;
  void setGoalNotifications(bool enabled);

  [[nodiscard]] QString goalDigestTime() const;
  void setGoalDigestTime(const QString &key);
  [[nodiscard]] int goalDigestMinutes() const;
  void setGoalDigestMinutes(int minutes);
  [[nodiscard]] bool goalDigestCatchUp() const;
  void setGoalDigestCatchUp(bool enabled);
  [[nodiscard]] activity::DigestPlan digestPlan() const;

  // The last day the summary was sent or dropped; no signal, only the
  // summary service writes it.
  [[nodiscard]] QDate goalDigestLastDay() const;
  void setGoalDigestLastDay(const QDate &day);

signals:
  void trackingEnabledChanged();
  void scheduleChanged();
  void languageChanged();
  void categoryRulesChanged();
  void privacyRulesChanged();
  void dailyGoalsChanged();
  void goalNotificationsChanged();
  void goalDigestChanged();

private:
  void store(const QString &key, const QVariant &value);

  std::unique_ptr<QSettings> _store;

  bool _trackingEnabled = true;
  activity::TrackingSchedule _schedule;
  QString _language;
  QList<activity::CategoryRule> _categoryRules;
  QList<activity::PrivacyRule> _privacyRules;
  QList<activity::DailyGoal> _dailyGoals;
  bool _goalNotifications = true;
  activity::DigestPlan _digestPlan;
  bool _digestCatchUp = true;
};

} // namespace chronexa::core
