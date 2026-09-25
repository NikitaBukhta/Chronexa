#include "domain/activity/Activity.hpp"
#include "fakes/Fakes.hpp"

#include <QTest>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

// Seconds past 10:00, so gaps under a second can be written down.
Activity row(const QString &app, const QString &title, int fromSecond,
             int toSecond, std::optional<QString> category = std::nullopt) {
  Activity activity;
  activity.appName = app;
  activity.title = title;
  activity.startedOn = utc(10, 0).addSecs(fromSecond);
  activity.endedOn = utc(10, 0).addSecs(toSecond);
  activity.category = std::move(category);
  return activity;
}

// Newest first, as the repository returns them.
QList<Activity> newestFirst(QList<Activity> rows) {
  std::reverse(rows.begin(), rows.end());
  return rows;
}

} // namespace

Q_DECLARE_METATYPE(SessionEdit)

class TestActivity : public QObject {
  Q_OBJECT

private slots:
  void flushCutsAreJoined();
  void joinStopsAtAGap();
  void joinStopsAtAnotherWindow();
  void joinStopsAtAnotherCategory();
  void joinToleratesAPollTick();
  void joinOfNothing();

  void editIsNormalized_data();
  void editIsNormalized();
  void editNeedsAnApplication();
};

void TestActivity::flushCutsAreJoined() {
  // One stretch in CLion from 10:00:00 to 10:02:30, cut by two flushes.
  const QList<Activity> joined = joinContiguous(newestFirst({
      row("CLion", "main.cpp", 0, 60),
      row("CLion", "main.cpp", 60, 120),
      row("CLion", "main.cpp", 120, 150),
  }));

  QCOMPARE(joined.size(), 1);
  QCOMPARE(joined.first().startedOn, utc(10, 0));
  QCOMPARE(joined.first().endedOn, utc(10, 2, 30));
  QCOMPARE(joined.first().durationSeconds(), 150);
}

void TestActivity::joinStopsAtAGap() {
  // Idle for a while in between: two sessions, not one.
  const QList<Activity> joined = joinContiguous(newestFirst({
      row("CLion", "main.cpp", 0, 60),
      row("CLion", "main.cpp", 90, 120),
  }));

  QCOMPARE(joined.size(), 2);
  QCOMPARE(joined.at(0).startedOn, utc(10, 1, 30));
  QCOMPARE(joined.at(1).endedOn, utc(10, 1));
}

void TestActivity::joinStopsAtAnotherWindow() {
  const QList<Activity> joined = joinContiguous(newestFirst({
      row("CLion", "main.cpp", 0, 60),
      row("CLion", "util.cpp", 60, 90),
      row("Slack", "util.cpp", 90, 100),
      row("CLion", "util.cpp", 100, 120),
  }));

  QCOMPARE(joined.size(), 4);
}

void TestActivity::joinStopsAtAnotherCategory() {
  // Half of the stretch was moved to another category by hand; the halves
  // are edited apart and have to stay apart.
  const QList<Activity> joined = joinContiguous(newestFirst({
      row("Chrome", "YouTube", 0, 60, QStringLiteral("Learning")),
      row("Chrome", "YouTube", 60, 120),
      row("Chrome", "YouTube", 120, 180, QStringLiteral("")),
  }));

  QCOMPARE(joined.size(), 3);
  QCOMPARE(joined.at(0).category, std::optional<QString>(QStringLiteral("")));
  QVERIFY(!joined.at(1).category.has_value());
  QCOMPARE(joined.at(2).category,
           std::optional<QString>(QStringLiteral("Learning")));
}

void TestActivity::joinToleratesAPollTick() {
  Activity first = row("CLion", "main.cpp", 0, 60);
  Activity second = row("CLion", "main.cpp", 60, 120);
  second.startedOn = second.startedOn.addMSecs(kJoinGapMs);
  QCOMPARE(joinContiguous({second, first}).size(), 1);

  second.startedOn = second.startedOn.addMSecs(1);
  QCOMPARE(joinContiguous({second, first}).size(), 2);
}

void TestActivity::joinOfNothing() {
  QVERIFY(joinContiguous({}).isEmpty());
  QCOMPARE(joinContiguous({row("CLion", "a", 0, 10)}).size(), 1);
}

void TestActivity::editIsNormalized_data() {
  QTest::addColumn<SessionEdit>("edit");
  QTest::addColumn<SessionEdit>("expected");

  QTest::newRow("trimmed")
      << SessionEdit{"  CLion ", " main.cpp  ", QStringLiteral(" Work ")}
      << SessionEdit{"CLion", "main.cpp", QStringLiteral("Work")};
  QTest::newRow("blank category is none")
      << SessionEdit{"CLion", "a", QStringLiteral("   ")}
      << SessionEdit{"CLion", "a", QStringLiteral("")};
  QTest::newRow("unset category stays unset")
      << SessionEdit{"CLion", "a", std::nullopt}
      << SessionEdit{"CLion", "a", std::nullopt};
}

void TestActivity::editIsNormalized() {
  QFETCH(SessionEdit, edit);
  QFETCH(SessionEdit, expected);

  const SessionEdit normalized = edit.normalized();
  QCOMPARE(normalized.appName, expected.appName);
  QCOMPARE(normalized.title, expected.title);
  QCOMPARE(normalized.category, expected.category);
}

void TestActivity::editNeedsAnApplication() {
  const SessionEdit named{"CLion", {}, std::nullopt};
  const SessionEdit blank{"   ", "main.cpp", std::nullopt};
  QVERIFY(named.isValid());
  QVERIFY(!blank.isValid());

  const SessionEdit untitled = SessionEdit{"CLion", QString(), {}}.normalized();
  QVERIFY2(!untitled.title.isNull(),
           "a null title would bind as NULL into a NOT NULL column");
}

QTEST_MAIN(TestActivity)
#include "tst_Activity.moc"
