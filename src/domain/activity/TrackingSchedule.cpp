#include "TrackingSchedule.hpp"

namespace chronexa::activity {

namespace {

constexpr int kSearchDays = 8;

} // namespace

bool TrackingSchedule::operator==(const TrackingSchedule &other) const {
  return enabled == other.enabled && start == other.start && end == other.end &&
         days == other.days;
}

bool TrackingSchedule::operator!=(const TrackingSchedule &other) const {
  return !(*this == other);
}

bool TrackingSchedule::overnight() const { return end < start; }

bool TrackingSchedule::includesDay(int dayOfWeek) const {
  if (dayOfWeek < 1 || dayOfWeek > 7) {
    return false;
  }
  return (days & (1 << (dayOfWeek - 1))) != 0;
}

bool TrackingSchedule::allows(const QDateTime &moment) const {
  if (!enabled) {
    return true;
  }
  if (!moment.isValid() || days == kNoDays || start == end) {
    return false;
  }

  const QTime time = moment.time();
  const int dayOfWeek = moment.date().dayOfWeek();

  if (!overnight()) {
    return includesDay(dayOfWeek) && time >= start && time < end;
  }

  // Overnight: the evening belongs to this day's window, the small hours to
  // the window that opened yesterday.
  if (time >= start) {
    return includesDay(dayOfWeek);
  }
  if (time < end) {
    return includesDay(dayOfWeek == 1 ? 7 : dayOfWeek - 1);
  }
  return false;
}

QDateTime TrackingSchedule::nextChange(const QDateTime &from) const {
  if (!enabled || !from.isValid() || days == kNoDays || start == end) {
    return {};
  }

  const bool now = allows(from);
  // Within one day the edges must be tried in clock order, otherwise an
  // overnight window reports its far edge before the near one.
  const QTime firstEdge = start < end ? start : end;
  const QTime secondEdge = start < end ? end : start;

  for (int dayOffset = 0; dayOffset <= kSearchDays; ++dayOffset) {
    const QDate date = from.date().addDays(dayOffset);
    for (const QTime &edge : {firstEdge, secondEdge}) {
      const QDateTime candidate(date, edge);
      if (candidate > from && allows(candidate) != now) {
        return candidate;
      }
    }
  }
  return {};
}

} // namespace chronexa::activity
