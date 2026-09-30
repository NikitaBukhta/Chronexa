#pragma once

#include "domain/activity/DailyGoals.hpp"

#include <QDate>
#include <QDateTime>
#include <QList>
#include <QObject>

#include <functional>

class QTimer;

namespace chronexa::activity {

class ActivityQueryService;

// Measures today's goals against the recorded history and tells when one is
// crossed: a limit gone past or a target reached. Each goal is announced at
// most once a day, and whatever was already crossed when the service first
// looked is not announced at all -- a restart must not repeat the day's news.
class GoalService : public QObject {
  Q_OBJECT

public:
  using Clock = std::function<QDateTime()>;

  explicit GoalService(const ActivityQueryService &queries,
                       QObject *parent = nullptr);
  // For tests: reads the time from `clock` and runs no timer of its own.
  GoalService(const ActivityQueryService &queries, Clock clock,
              QObject *parent = nullptr);

  void setGoals(const QList<DailyGoal> &goals);
  [[nodiscard]] const QList<DailyGoal> &goals() const;

  // The goals that apply today, with the time spent on them so far.
  [[nodiscard]] const QList<GoalProgress> &progress() const;
  [[nodiscard]] QDate day() const;

public slots:
  void refresh();

signals:
  void progressChanged();
  void goalCrossed(const chronexa::activity::GoalProgress &progress);

private:
  void refreshOnNewDay();

  const ActivityQueryService &_queries;
  Clock _clock;
  QTimer *_dayTimer = nullptr;

  QList<DailyGoal> _goals;
  QList<GoalProgress> _progress;
  QDate _day;
  // Goals already announced (or found crossed at start-up) on `_day`.
  QList<DailyGoal> _announced;
  bool _primed = false;
};

} // namespace chronexa::activity
