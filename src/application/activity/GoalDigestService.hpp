#pragma once

#include "domain/activity/DailyGoals.hpp"
#include "domain/activity/GoalDigest.hpp"
#include "domain/activity/TrackingSchedule.hpp"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QObject>

#include <functional>

class QTimer;

namespace chronexa::activity {

class ActivityQueryService;

// Sends the daily goal summary once a day, when the plan says. One whose time
// passed unsent -- the app was closed, the machine asleep -- is overdue: it is
// sent as soon as the app notices, on launch or on waking, unless catching up
// is off, in which case that day's summary is dropped.
class GoalDigestService : public QObject {
  Q_OBJECT

public:
  using Clock = std::function<QDateTime()>;

  explicit GoalDigestService(const ActivityQueryService &queries,
                             QObject *parent = nullptr);
  // For tests: reads the time from `clock`; nothing runs until check().
  GoalDigestService(const ActivityQueryService &queries, Clock clock,
                    QObject *parent = nullptr);

  void setGoals(const QList<DailyGoal> &goals);
  void setPlan(const DigestPlan &plan);
  void setSchedule(const TrackingSchedule &schedule);
  void setCatchUp(bool enabled);
  // The last day a summary was sent (or dropped); kept by the caller.
  void setLastSentDay(const QDate &day);

  [[nodiscard]] QDate lastSentDay() const;
  // When the next summary is due, not counting an overdue one.
  [[nodiscard]] QDateTime nextDue() const;

  // Checks once a minute from now on; the first look -- where a summary
  // missed while the app was closed shows up -- follows after `delayMs`, so
  // the tray icon has settled before it is used.
  void start(int delayMs);

public slots:
  void check();

signals:
  void digestDue(const chronexa::activity::GoalDigest &digest);
  void lastSentDayChanged(const QDate &day);
  void nextDueChanged();

private:
  void markSent(const QDate &day);

  const ActivityQueryService &_queries;
  Clock _clock;
  QTimer *_timer = nullptr;

  QList<DailyGoal> _goals;
  DigestPlan _plan;
  TrackingSchedule _schedule;
  bool _catchUp = true;
  QDate _lastSentDay;
};

} // namespace chronexa::activity
