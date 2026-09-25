#include "application/activity/ActivityQueryService.hpp"
#include "fakes/Fakes.hpp"

#include <QTest>
#include <QTimeZone>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const QDateTime kFrom(QDate(2026, 9, 1), QTime(0, 0), QTimeZone::UTC);
const QDateTime kTo(QDate(2026, 9, 2), QTime(0, 0), QTimeZone::UTC);

} // namespace

class TestActivityQueryService : public QObject {
  Q_OBJECT

private slots:
  void noRulesSkipsTheQuery();
  void categoriesFollowTheRules();
  void ruleChangeAppliesRetroactively();
  void sessionsAreJoined();
  void categoryOfPrefersTheOneSetByHand();
};

void TestActivityQueryService::noRulesSkipsTheQuery() {
  FakeActivityRepository repository;
  repository.titles = {
      TitleTotal{QStringLiteral("CLion"), QString(), 60'000, 1}};
  ActivityQueryService service(repository);

  QVERIFY(service.categoryTotals(kFrom, kTo).isEmpty());
  QCOMPARE(repository.titleTotalsCalls, 0);
}

void TestActivityQueryService::categoriesFollowTheRules() {
  FakeActivityRepository repository;
  repository.titles = {
      TitleTotal{QStringLiteral("Google Chrome"), QStringLiteral("YouTube"),
                 120'000, 1},
      TitleTotal{QStringLiteral("Google Chrome"), QStringLiteral("Jira"),
                 60'000, 1},
      TitleTotal{QStringLiteral("CLion"), QStringLiteral("main.cpp"), 300'000,
                 1},
  };
  ActivityQueryService service(repository);
  service.setCategoryRules(CategoryRules({
      CategoryRule{QStringLiteral("Distractions"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
      CategoryRule{QStringLiteral("Work"), {QStringLiteral("CLion")}, {}},
  }));

  const QList<CategoryTotal> totals = service.categoryTotals(kFrom, kTo);

  QCOMPARE(repository.lastFrom, kFrom);
  QCOMPARE(repository.lastTo, kTo);
  QCOMPARE(totals.size(), 3);
  QCOMPARE(totals.at(0).category, QStringLiteral("Work"));
  QCOMPARE(totals.at(0).seconds, 300);
  QCOMPARE(totals.at(1).category, QStringLiteral("Distractions"));
  QCOMPARE(totals.at(1).seconds, 120);
  QCOMPARE(totals.at(2).category, QString());
  QCOMPARE(totals.at(2).seconds, 60);
}

void TestActivityQueryService::ruleChangeAppliesRetroactively() {
  FakeActivityRepository repository;
  repository.titles = {TitleTotal{QStringLiteral("Google Chrome"),
                                  QStringLiteral("Jira"), 60'000, 1}};
  ActivityQueryService service(repository);

  service.setCategoryRules(CategoryRules(
      {CategoryRule{QStringLiteral("Work"), {QStringLiteral("CLion")}, {}}}));
  QCOMPARE(service.categoryTotals(kFrom, kTo).first().category, QString());

  service.setCategoryRules(CategoryRules(
      {CategoryRule{QStringLiteral("Work"), {}, QStringLiteral("jira")}}));
  QCOMPARE(service.categoryTotals(kFrom, kTo).first().category,
           QStringLiteral("Work"));
}

void TestActivityQueryService::sessionsAreJoined() {
  FakeActivityRepository repository;
  const QString app = QStringLiteral("CLion");
  // Newest first, as stored: one stretch cut by two flushes, then a gap.
  repository.inserted = {
      Activity{app, QStringLiteral("a"), utc(10, 2), utc(10, 2, 40)},
      Activity{app, QStringLiteral("a"), utc(10, 1), utc(10, 2)},
      Activity{app, QStringLiteral("a"), utc(10, 0, 20), utc(10, 1)},
      Activity{app, QStringLiteral("a"), utc(9, 0), utc(9, 1)},
  };
  ActivityQueryService service(repository);

  const QList<Activity> sessions = service.sessions(kFrom, kTo);
  QCOMPARE(sessions.size(), 2);
  QCOMPARE(sessions.first().startedOn, utc(10, 0, 20));
  QCOMPARE(sessions.first().endedOn, utc(10, 2, 40));
}

void TestActivityQueryService::categoryOfPrefersTheOneSetByHand() {
  FakeActivityRepository repository;
  ActivityQueryService service(repository);
  service.setCategoryRules(CategoryRules({
      CategoryRule{QStringLiteral("Distractions"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
  }));

  Activity video{QStringLiteral("Google Chrome"), QStringLiteral("YouTube"),
                 utc(10, 0), utc(10, 5)};
  QCOMPARE(service.categoryOf(video), QStringLiteral("Distractions"));
  video.category = QStringLiteral("Learning");
  QCOMPARE(service.categoryOf(video), QStringLiteral("Learning"));
  video.category = QStringLiteral("");
  QVERIFY2(service.categoryOf(video).isEmpty(),
           "taken out of every category by hand");
}

QTEST_MAIN(TestActivityQueryService)
#include "tst_ActivityQueryService.moc"
