#include "domain/activity/GoalDigest.hpp"

#include <QTest>

using namespace chronexa::activity;

namespace {

const QString kDistractions = QStringLiteral("Distractions");
const QString kWork = QStringLiteral("Work");

// 2026-09-21 is a Monday.
QDateTime on(int dayOffset, int hour, int minute = 0) {
  return QDateTime(QDate(2026, 9, 21).addDays(dayOffset), QTime(hour, minute));
}

TrackingSchedule workHours(bool enabled) {
  TrackingSchedule schedule;
  schedule.enabled = enabled;
  schedule.start = QTime(8, 30);
  schedule.end = QTime(17, 30);
  schedule.days = TrackingSchedule::kWorkdays;
  return schedule;
}

DigestPlan plan(DigestTime when, QTime at = QTime(9, 0)) {
  return {when, at};
}

} // namespace

Q_DECLARE_METATYPE(chronexa::activity::DigestPlan)

class TestGoalDigest : public QObject {
  Q_OBJECT

private slots:
  void keys();
  void dueOn_data();
  void dueOn();
  void nextDue_data();
  void nextDue();
  void overdueAfterTheGrace();
  void digestReportsYesterdayAndListsToday();
  void emptyWithoutGoals();
};

void TestGoalDigest::keys() {
  for (const DigestTime when :
       {DigestTime::WorkStart, DigestTime::Custom, DigestTime::Off}) {
    QCOMPARE(digestTimeFromKey(digestTimeKey(when)), when);
  }
  QVERIFY2(digestTimeFromKey(QStringLiteral("junk")) == DigestTime::WorkStart,
           "an unknown or missing value is the default");
  QCOMPARE(digestTimeFromKey(QString()), DigestTime::WorkStart);
}

void TestGoalDigest::dueOn_data() {
  QTest::addColumn<DigestPlan>("digestPlan");
  QTest::addColumn<bool>("scheduleOn");
  QTest::addColumn<int>("dayOffset");
  QTest::addColumn<QDateTime>("expected");

  QTest::newRow("work start, a workday")
      << plan(DigestTime::WorkStart) << true << 0 << on(0, 8, 30);
  QTest::newRow("work start, Saturday")
      << plan(DigestTime::WorkStart) << true << 5 << QDateTime();
  QTest::newRow("work start, schedule off: every day")
      << plan(DigestTime::WorkStart) << false << 5 << on(5, 8, 30);
  QTest::newRow("set time, Sunday")
      << plan(DigestTime::Custom, QTime(7, 45)) << true << 6 << on(6, 7, 45);
  QTest::newRow("off") << plan(DigestTime::Off) << true << 0 << QDateTime();
}

void TestGoalDigest::dueOn() {
  QFETCH(DigestPlan, digestPlan);
  QFETCH(bool, scheduleOn);
  QFETCH(int, dayOffset);
  QFETCH(QDateTime, expected);

  QCOMPARE(digestPlan.dueOn(on(dayOffset, 0).date(), workHours(scheduleOn)),
           expected);
}

void TestGoalDigest::nextDue_data() {
  QTest::addColumn<DigestPlan>("digestPlan");
  QTest::addColumn<QDateTime>("from");
  QTest::addColumn<QDateTime>("expected");

  QTest::newRow("before today's")
      << plan(DigestTime::WorkStart) << on(0, 7) << on(0, 8, 30);
  QTest::newRow("exactly at it")
      << plan(DigestTime::WorkStart) << on(0, 8, 30) << on(0, 8, 30);
  QTest::newRow("Friday afternoon: Monday")
      << plan(DigestTime::WorkStart) << on(4, 14) << on(7, 8, 30);
  QTest::newRow("set time, after today's: tomorrow")
      << plan(DigestTime::Custom, QTime(9, 0)) << on(5, 10) << on(6, 9);
  QTest::newRow("off") << plan(DigestTime::Off) << on(0, 7) << QDateTime();
}

void TestGoalDigest::nextDue() {
  QFETCH(DigestPlan, digestPlan);
  QFETCH(QDateTime, from);
  QFETCH(QDateTime, expected);

  QCOMPARE(digestPlan.nextDue(from, workHours(true)), expected);
}

void TestGoalDigest::overdueAfterTheGrace() {
  const QDateTime due = on(0, 9);
  QVERIFY(!isOverdue(due, due));
  QVERIFY2(!isOverdue(due, due.addSecs(kDigestGraceSeconds)),
           "a late timer tick is not a missed summary");
  QVERIFY(isOverdue(due, due.addSecs(kDigestGraceSeconds + 1)));
  QVERIFY(isOverdue(due, on(0, 14)));
  QVERIFY(!isOverdue(QDateTime(), on(0, 14)));
}

void TestGoalDigest::digestReportsYesterdayAndListsToday() {
  DailyGoal weekdays{kWork, GoalKind::Target, 240, TrackingSchedule::kWorkdays};
  const DailyGoal limit{kDistractions, GoalKind::Limit, 60,
                        TrackingSchedule::kEveryDay};

  // Tuesday morning: Monday is reported, both goals apply today.
  const GoalDigest tuesday =
      makeDigest({weekdays, limit}, on(1, 0).date(),
                 {{kWork, 5 * 3600, 9}, {kDistractions, 80 * 60, 4}});
  QCOMPARE(tuesday.reportedDay, on(0, 0).date());
  QCOMPARE(tuesday.results.size(), 2);
  QCOMPARE(tuesday.metCount(), 1);
  QVERIFY(isMet(tuesday.results[0]));
  QVERIFY(!isMet(tuesday.results[1]));
  QCOMPARE(tuesday.today, (QList<DailyGoal>{weekdays, limit}));
  QVERIFY(!tuesday.overdue);

  // Sunday: Saturday had only the limit, and only the limit applies today.
  const GoalDigest sunday =
      makeDigest({weekdays, limit}, on(6, 0).date(), {});
  QCOMPARE(sunday.results.size(), 1);
  QCOMPARE(sunday.metCount(), 1);
  QCOMPARE(sunday.today, QList<DailyGoal>{limit});
}

void TestGoalDigest::emptyWithoutGoals() {
  QVERIFY(makeDigest({}, on(1, 0).date(), {{kWork, 3600, 1}}).isEmpty());

  const DailyGoal weekend{kWork, GoalKind::Target, 60, 0b1100000};
  // Tuesday: the weekend goal neither applied yesterday nor applies today.
  QVERIFY(makeDigest({weekend}, on(1, 0).date(), {}).isEmpty());
}

QTEST_APPLESS_MAIN(TestGoalDigest)
#include "tst_GoalDigest.moc"
