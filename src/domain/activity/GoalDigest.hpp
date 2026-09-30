#pragma once

#include "DailyGoals.hpp"
#include "TrackingSchedule.hpp"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QTime>

namespace chronexa::activity {

enum class DigestTime {
  // When the tracking schedule's hours start, on its days; every day at the
  // schedule's start time while the schedule itself is off.
  WorkStart,
  // Every day at a set time.
  Custom,
  Off,
};

QString digestTimeKey(DigestTime when);
DigestTime digestTimeFromKey(const QString &key);

// When the daily goal summary is due.
struct DigestPlan {
  DigestTime when = DigestTime::WorkStart;
  QTime at = QTime(9, 0);

  bool operator==(const DigestPlan &other) const;
  bool operator!=(const DigestPlan &other) const;

  // Invalid when no summary is due on `day`.
  [[nodiscard]] QDateTime dueOn(const QDate &day,
                                const TrackingSchedule &schedule) const;
  // The first due moment at or after `from`, within the next week.
  [[nodiscard]] QDateTime nextDue(const QDateTime &from,
                                  const TrackingSchedule &schedule) const;
};

// A summary delivered later than this after its due time counts as overdue.
constexpr qint64 kDigestGraceSeconds = 10 * 60;

[[nodiscard]] bool isOverdue(const QDateTime &due, const QDateTime &now);

// Yesterday's goals as they ended, and the goals that apply today.
struct GoalDigest {
  QDate reportedDay;
  QList<GoalProgress> results;
  QList<DailyGoal> today;
  // Delivered after its time: on launch or after the machine woke up.
  bool overdue = false;

  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] int metCount() const;
};

// A limit kept or a target reached.
[[nodiscard]] bool isMet(const GoalProgress &progress);

GoalDigest makeDigest(const QList<DailyGoal> &goals, const QDate &today,
                      const QList<CategoryTotal> &yesterdayTotals);

} // namespace chronexa::activity
