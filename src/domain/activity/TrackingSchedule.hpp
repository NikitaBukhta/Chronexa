#pragma once

#include <QDateTime>
#include <QTime>

namespace chronexa::activity {

struct TrackingSchedule {
  static constexpr int kNoDays = 0;
  static constexpr int kEveryDay = 0b1111111;
  static constexpr int kWorkdays = 0b0011111;

  bool enabled = false;
  QTime start = QTime(9, 0);
  QTime end = QTime(18, 0);
  int days = kWorkdays;

  bool operator==(const TrackingSchedule &other) const;
  bool operator!=(const TrackingSchedule &other) const;

  bool overnight() const;

  bool includesDay(int dayOfWeek) const;

  bool allows(const QDateTime &moment) const;

  QDateTime nextChange(const QDateTime &from) const;
};

} // namespace chronexa::activity
