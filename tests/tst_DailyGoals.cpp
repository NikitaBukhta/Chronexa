#include "domain/activity/DailyGoals.hpp"

#include <QTest>

using namespace chronexa::activity;

namespace {

const QString kDistractions = QStringLiteral("Distractions");
const QString kWork = QStringLiteral("Work");

// 2026-09-21 is a Monday.
const QDate kMonday(2026, 9, 21);
const QDate kSaturday(2026, 9, 26);

DailyGoal limit(const QString &category, int minutes) {
  return {category, GoalKind::Limit, minutes, TrackingSchedule::kEveryDay};
}

DailyGoal target(const QString &category, int minutes) {
  return {category, GoalKind::Target, minutes, TrackingSchedule::kEveryDay};
}

} // namespace

Q_DECLARE_METATYPE(chronexa::activity::DailyGoal)
Q_DECLARE_METATYPE(chronexa::activity::GoalKind)
Q_DECLARE_METATYPE(chronexa::activity::GoalState)

class TestDailyGoals : public QObject {
  Q_OBJECT

private slots:
  void state_data();
  void state();
  void remainingAndOver();
  void validity_data();
  void validity();
  void appliesOnMaskedDays();
  void progressMeasuresEachGoal();
  void progressSkipsInvalidAndOffDays();
  void progressKeepsGoalOrder();
  void roundTrip();
  void parseToleratesJunk();
};

void TestDailyGoals::state_data() {
  QTest::addColumn<GoalKind>("kind");
  QTest::addColumn<qint64>("seconds");
  QTest::addColumn<GoalState>("expected");
  QTest::addColumn<bool>("crossed");

  // A one-hour goal throughout.
  QTest::newRow("limit, nothing yet")
      << GoalKind::Limit << qint64(0) << GoalState::Within << false;
  QTest::newRow("limit, exactly at it")
      << GoalKind::Limit << qint64(3600) << GoalState::Within << false;
  QTest::newRow("limit, a second past")
      << GoalKind::Limit << qint64(3601) << GoalState::Exceeded << true;
  QTest::newRow("target, a second short")
      << GoalKind::Target << qint64(3599) << GoalState::Short << false;
  QTest::newRow("target, exactly at it")
      << GoalKind::Target << qint64(3600) << GoalState::Reached << true;
  QTest::newRow("target, beyond it")
      << GoalKind::Target << qint64(7200) << GoalState::Reached << true;
}

void TestDailyGoals::state() {
  QFETCH(GoalKind, kind);
  QFETCH(qint64, seconds);
  QFETCH(GoalState, expected);
  QFETCH(bool, crossed);

  const GoalProgress progress{{kWork, kind, 60, TrackingSchedule::kEveryDay},
                              seconds};
  QCOMPARE(progress.state(), expected);
  QCOMPARE(progress.crossed(), crossed);
  QCOMPARE(progress.fraction(), seconds / 3600.0);
}

void TestDailyGoals::remainingAndOver() {
  const GoalProgress under{limit(kDistractions, 60), 45 * 60};
  QCOMPARE(under.remainingSeconds(), qint64(15 * 60));
  QCOMPARE(under.overSeconds(), qint64(0));

  const GoalProgress over{limit(kDistractions, 60), 72 * 60};
  QCOMPARE(over.remainingSeconds(), qint64(0));
  QCOMPARE(over.overSeconds(), qint64(12 * 60));
}

void TestDailyGoals::validity_data() {
  QTest::addColumn<DailyGoal>("goal");
  QTest::addColumn<bool>("valid");

  QTest::newRow("plain") << limit(kWork, 60) << true;
  QTest::newRow("no category") << limit(QString(), 60) << false;
  QTest::newRow("blank category") << limit(QStringLiteral("  "), 60) << false;
  QTest::newRow("zero minutes") << limit(kWork, 0) << false;
  QTest::newRow("a whole day and more")
      << limit(kWork, DailyGoal::kMaxMinutes + 1) << false;
  QTest::newRow("longest") << limit(kWork, DailyGoal::kMaxMinutes) << true;
  DailyGoal noDays = limit(kWork, 60);
  noDays.days = TrackingSchedule::kNoDays;
  QTest::newRow("no days") << noDays << false;
}

void TestDailyGoals::validity() {
  QFETCH(DailyGoal, goal);
  QFETCH(bool, valid);
  QCOMPARE(goal.isValid(), valid);
}

void TestDailyGoals::appliesOnMaskedDays() {
  DailyGoal goal = target(kWork, 240);
  goal.days = TrackingSchedule::kWorkdays;

  QVERIFY(goal.appliesOn(kMonday));
  QVERIFY(goal.appliesOn(kMonday.addDays(4)));
  QVERIFY(!goal.appliesOn(kSaturday));
  QVERIFY(!goal.appliesOn(kSaturday.addDays(1)));
  QVERIFY(!goal.appliesOn(QDate()));
}

void TestDailyGoals::progressMeasuresEachGoal() {
  const QList<CategoryTotal> totals{
      {kWork, 3 * 3600, 12},
      {kDistractions, 50 * 60, 4},
      {QString(), 600, 2},
  };

  const QList<GoalProgress> progress = goalProgress(
      {limit(kDistractions, 60), target(kWork, 240),
       limit(QStringLiteral("Communication"), 30)},
      kMonday, totals);

  QCOMPARE(progress.size(), 3);
  QCOMPARE(progress[0].seconds, qint64(50 * 60));
  QCOMPARE(progress[0].state(), GoalState::Within);
  QCOMPARE(progress[1].seconds, qint64(3 * 3600));
  QCOMPARE(progress[1].state(), GoalState::Short);
  QVERIFY2(progress[2].seconds == 0,
           "a category with no time today counts as zero, not as missing");
}

void TestDailyGoals::progressSkipsInvalidAndOffDays() {
  DailyGoal weekdays = target(kWork, 240);
  weekdays.days = TrackingSchedule::kWorkdays;

  const QList<GoalProgress> progress = goalProgress(
      {weekdays, limit(QString(), 60), limit(kDistractions, 60)}, kSaturday,
      {{kDistractions, 90 * 60, 3}});

  QCOMPARE(progress.size(), 1);
  QCOMPARE(progress[0].goal.category, kDistractions);
  QCOMPARE(progress[0].state(), GoalState::Exceeded);
}

void TestDailyGoals::progressKeepsGoalOrder() {
  // Totals arrive sorted by time; goals keep the order the user gave them.
  const QList<GoalProgress> progress =
      goalProgress({target(kWork, 60), limit(kDistractions, 60)}, kMonday,
                   {{kDistractions, 7200, 1}, {kWork, 60, 1}});
  QCOMPARE(progress[0].goal.category, kWork);
  QCOMPARE(progress[1].goal.category, kDistractions);
}

void TestDailyGoals::roundTrip() {
  DailyGoal weekdays = target(QStringLiteral("Работа"), 240);
  weekdays.days = TrackingSchedule::kWorkdays;
  const QList<DailyGoal> goals{limit(kDistractions, 60), weekdays};

  QCOMPARE(parseDailyGoals(serializeDailyGoals(goals)), goals);
  QCOMPARE(parseDailyGoals(serializeDailyGoals({})), QList<DailyGoal>());
}

void TestDailyGoals::parseToleratesJunk() {
  QVERIFY(parseDailyGoals(QString()).isEmpty());
  QVERIFY(parseDailyGoals(QStringLiteral("{\"not\":\"a list\"}")).isEmpty());

  const QList<DailyGoal> goals = parseDailyGoals(QStringLiteral(
      R"([42, {"category":"Work","kind":"sideways","minutes":99999,"days":1023},
          {"category":"Fun"}])"));
  QCOMPARE(goals.size(), 2);
  QCOMPARE(goals[0].kind, GoalKind::Limit);
  QCOMPARE(goals[0].minutes, DailyGoal::kMaxMinutes);
  QCOMPARE(goals[0].days, TrackingSchedule::kEveryDay);
  QVERIFY2(goals[1] == limit(QStringLiteral("Fun"), 60),
           "missing fields fall back to the defaults");
}

QTEST_APPLESS_MAIN(TestDailyGoals)
#include "tst_DailyGoals.moc"
