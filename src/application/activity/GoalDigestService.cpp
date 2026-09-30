#include "GoalDigestService.hpp"
#include "ActivityQueryService.hpp"

#include <QLoggingCategory>
#include <QTimeZone>
#include <QTimer>

namespace {

Q_LOGGING_CATEGORY(lcDigest, "chronexa.activity.digest")

constexpr int kCheckMs = 60 * 1000;

} // namespace

namespace chronexa::activity {

GoalDigestService::GoalDigestService(const ActivityQueryService &queries,
                                     QObject *parent)
    : GoalDigestService(
          queries, []() { return QDateTime::currentDateTime(); }, parent) {}

GoalDigestService::GoalDigestService(const ActivityQueryService &queries,
                                     Clock clock, QObject *parent)
    : QObject(parent), _queries(queries), _clock(std::move(clock)) {}

void GoalDigestService::setGoals(const QList<DailyGoal> &goals) {
  _goals = goals;
}

void GoalDigestService::setPlan(const DigestPlan &plan) {
  if (_plan == plan) {
    return;
  }
  _plan = plan;
  emit nextDueChanged();
}

void GoalDigestService::setSchedule(const TrackingSchedule &schedule) {
  if (_schedule == schedule) {
    return;
  }
  _schedule = schedule;
  emit nextDueChanged();
}

void GoalDigestService::setCatchUp(bool enabled) { _catchUp = enabled; }

void GoalDigestService::setLastSentDay(const QDate &day) { _lastSentDay = day; }

QDate GoalDigestService::lastSentDay() const { return _lastSentDay; }

QDateTime GoalDigestService::nextDue() const {
  const QDateTime now = _clock();
  const QDateTime today = _plan.dueOn(now.date(), _schedule);
  // Today's is still ahead, or already dealt with: look from tomorrow on.
  if (today.isValid() && today >= now) {
    return today;
  }
  return _plan.nextDue(QDateTime(now.date().addDays(1), QTime(0, 0)),
                       _schedule);
}

void GoalDigestService::start(int delayMs) {
  if (_timer == nullptr) {
    _timer = new QTimer(this);
    _timer->setInterval(kCheckMs);
    connect(_timer, &QTimer::timeout, this, &GoalDigestService::check);
  }
  _timer->start();
  QTimer::singleShot(delayMs, this, &GoalDigestService::check);
}

void GoalDigestService::check() {
  const QDateTime now = _clock();
  const QDate today = now.date();
  if (_lastSentDay == today) {
    return;
  }
  const QDateTime due = _plan.dueOn(today, _schedule);
  if (!due.isValid() || now < due) {
    return;
  }

  const bool overdue = isOverdue(due, now);
  if (overdue && !_catchUp) {
    qCInfo(lcDigest) << "Summary due at" << due << "missed; catching up is off";
    markSent(today);
    return;
  }

  const QDateTime from(today.addDays(-1), QTime(0, 0), now.timeZone());
  GoalDigest digest =
      makeDigest(_goals, today, _queries.categoryTotals(from, from.addDays(1)));
  digest.overdue = overdue;
  markSent(today);
  if (digest.isEmpty()) {
    qCInfo(lcDigest) << "No goals yesterday or today -- no summary";
    return;
  }

  qCInfo(lcDigest) << "Summary sent" << (overdue ? "late" : "on time") << "for"
                   << digest.reportedDay << ":" << digest.metCount() << "of"
                   << digest.results.size() << "met";
  emit digestDue(digest);
}

void GoalDigestService::markSent(const QDate &day) {
  _lastSentDay = day;
  emit lastSentDayChanged(day);
  emit nextDueChanged();
}

} // namespace chronexa::activity
