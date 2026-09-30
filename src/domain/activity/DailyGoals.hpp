#pragma once

#include "CategoryRules.hpp"
#include "TrackingSchedule.hpp"

#include <QDate>
#include <QList>
#include <QString>

namespace chronexa::activity {

enum class GoalKind {
  // At most this much a day: "no more than an hour of distractions".
  Limit,
  // At least this much a day: "four hours of work".
  Target,
};

QString goalKindKey(GoalKind kind);
GoalKind goalKindFromKey(const QString &key);

struct DailyGoal {
  static constexpr int kMaxMinutes = 24 * 60 - 1;

  QString category;
  GoalKind kind = GoalKind::Limit;
  int minutes = 60;
  // Weekday mask, as in TrackingSchedule: bit 0 is Monday.
  int days = TrackingSchedule::kEveryDay;

  bool operator==(const DailyGoal &other) const;
  bool operator!=(const DailyGoal &other) const;

  [[nodiscard]] bool isValid() const;
  [[nodiscard]] bool appliesOn(const QDate &date) const;
  [[nodiscard]] qint64 thresholdSeconds() const;
};

enum class GoalState {
  // A limit with room left.
  Within,
  // A limit gone past.
  Exceeded,
  // A target not reached yet.
  Short,
  // A target reached.
  Reached,
};

struct GoalProgress {
  DailyGoal goal;
  qint64 seconds = 0;

  bool operator==(const GoalProgress &other) const;
  bool operator!=(const GoalProgress &other) const;

  [[nodiscard]] GoalState state() const;
  // The moment worth telling the user about: a limit gone past or a target
  // reached.
  [[nodiscard]] bool crossed() const;
  // Spent time over the threshold; above 1 once a goal is past it.
  [[nodiscard]] double fraction() const;
  // Left before a limit, or still missing for a target; never negative.
  [[nodiscard]] qint64 remainingSeconds() const;
  // How far past a limit (or beyond a target) the day went.
  [[nodiscard]] qint64 overSeconds() const;
};

// The valid goals that apply on `day`, measured against that day's category
// totals, in the order the goals were given.
QList<GoalProgress> goalProgress(const QList<DailyGoal> &goals,
                                 const QDate &day,
                                 const QList<CategoryTotal> &totals);

QString serializeDailyGoals(const QList<DailyGoal> &goals);
QList<DailyGoal> parseDailyGoals(const QString &json);

} // namespace chronexa::activity
