#include "GoalDigest.hpp"

#include <algorithm>

namespace {

constexpr auto kWorkStartKey = "workstart";
constexpr auto kCustomKey = "custom";
constexpr auto kOffKey = "off";

constexpr int kSearchDays = 8;

} // namespace

namespace chronexa::activity {

QString digestTimeKey(DigestTime when) {
  switch (when) {
  case DigestTime::Custom:
    return QLatin1String(kCustomKey);
  case DigestTime::Off:
    return QLatin1String(kOffKey);
  case DigestTime::WorkStart:
    break;
  }
  return QLatin1String(kWorkStartKey);
}

DigestTime digestTimeFromKey(const QString &key) {
  if (key == QLatin1String(kCustomKey)) {
    return DigestTime::Custom;
  }
  if (key == QLatin1String(kOffKey)) {
    return DigestTime::Off;
  }
  return DigestTime::WorkStart;
}

bool DigestPlan::operator==(const DigestPlan &other) const {
  return when == other.when && at == other.at;
}

bool DigestPlan::operator!=(const DigestPlan &other) const {
  return !(*this == other);
}

QDateTime DigestPlan::dueOn(const QDate &day,
                            const TrackingSchedule &schedule) const {
  if (!day.isValid()) {
    return {};
  }
  switch (when) {
  case DigestTime::Off:
    return {};
  case DigestTime::Custom:
    return at.isValid() ? QDateTime(day, at) : QDateTime();
  case DigestTime::WorkStart:
    break;
  }

  if (!schedule.start.isValid()) {
    return {};
  }
  // A day off has no start of work. With the schedule off every day is
  // tracked, and the start time still says when the day begins.
  if (schedule.enabled && !schedule.includesDay(day.dayOfWeek())) {
    return {};
  }
  return QDateTime(day, schedule.start);
}

QDateTime DigestPlan::nextDue(const QDateTime &from,
                              const TrackingSchedule &schedule) const {
  if (!from.isValid()) {
    return {};
  }
  for (int offset = 0; offset < kSearchDays; ++offset) {
    const QDateTime due = dueOn(from.date().addDays(offset), schedule);
    if (due.isValid() && due >= from) {
      return due;
    }
  }
  return {};
}

bool isOverdue(const QDateTime &due, const QDateTime &now) {
  return due.isValid() && now.isValid() &&
         due.secsTo(now) > kDigestGraceSeconds;
}

bool GoalDigest::isEmpty() const {
  return results.isEmpty() && today.isEmpty();
}

int GoalDigest::metCount() const {
  return static_cast<int>(
      std::count_if(results.cbegin(), results.cend(), &isMet));
}

bool isMet(const GoalProgress &progress) {
  const GoalState state = progress.state();
  return state == GoalState::Within || state == GoalState::Reached;
}

GoalDigest makeDigest(const QList<DailyGoal> &goals, const QDate &today,
                      const QList<CategoryTotal> &yesterdayTotals) {
  GoalDigest digest;
  digest.reportedDay = today.addDays(-1);
  digest.results = goalProgress(goals, digest.reportedDay, yesterdayTotals);
  for (const DailyGoal &goal : goals) {
    if (goal.isValid() && goal.appliesOn(today)) {
      digest.today.append(goal);
    }
  }
  return digest;
}

} // namespace chronexa::activity
