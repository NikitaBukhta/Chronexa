#include "GoalService.hpp"
#include "ActivityQueryService.hpp"

#include <QLoggingCategory>
#include <QTimeZone>
#include <QTimer>

namespace {

Q_LOGGING_CATEGORY(lcGoals, "chronexa.activity.goals")

constexpr int kDayCheckMs = 60 * 1000;

} // namespace

namespace chronexa::activity {

GoalService::GoalService(const ActivityQueryService &queries, QObject *parent)
    : GoalService(
          queries, []() { return QDateTime::currentDateTime(); }, parent) {
  // A day with nothing recorded after midnight must still start over, so the
  // day is watched on its own and not only through new activity.
  _dayTimer = new QTimer(this);
  _dayTimer->setInterval(kDayCheckMs);
  connect(_dayTimer, &QTimer::timeout, this, &GoalService::refreshOnNewDay);
  _dayTimer->start();
}

GoalService::GoalService(const ActivityQueryService &queries, Clock clock,
                         QObject *parent)
    : QObject(parent), _queries(queries), _clock(std::move(clock)) {}

void GoalService::setGoals(const QList<DailyGoal> &goals) {
  if (_goals == goals && _primed) {
    return;
  }
  _goals = goals;
  qCInfo(lcGoals) << "Goals set:" << _goals.size();
  refresh();
}

const QList<DailyGoal> &GoalService::goals() const { return _goals; }

const QList<GoalProgress> &GoalService::progress() const { return _progress; }

QDate GoalService::day() const { return _day; }

void GoalService::refresh() {
  const QDateTime now = _clock();
  const QDate today = now.date();
  if (today != _day) {
    _day = today;
    _announced.clear();
  }

  const QDateTime from(today, QTime(0, 0), now.timeZone());
  const QDateTime to = from.addDays(1);
  QList<GoalProgress> progress =
      goalProgress(_goals, today, _queries.categoryTotals(from, to));

  QList<GoalProgress> crossed;
  for (const GoalProgress &item : std::as_const(progress)) {
    if (!item.crossed() || _announced.contains(item.goal)) {
      continue;
    }
    _announced.append(item.goal);
    if (_primed) {
      crossed.append(item);
    }
  }
  _primed = true;

  if (progress != _progress) {
    _progress = std::move(progress);
    emit progressChanged();
  }

  for (const GoalProgress &item : std::as_const(crossed)) {
    qCInfo(lcGoals) << "Goal crossed:" << item.goal.category
                    << goalKindKey(item.goal.kind) << item.goal.minutes
                    << "min, spent" << item.seconds << "s";
    emit goalCrossed(item);
  }
}

void GoalService::refreshOnNewDay() {
  if (_clock().date() != _day) {
    refresh();
  }
}

} // namespace chronexa::activity
