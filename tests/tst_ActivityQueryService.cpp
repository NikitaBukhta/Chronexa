#include "application/activity/ActivityQueryService.hpp"

#include <QTest>
#include <QTimeZone>

using namespace chronexa::activity;

namespace {

// Serves canned title totals and records what it was asked for.
class FakeActivityRepository : public IActivityRepository {
public:
  QList<TitleTotal> titles;
  mutable int titleTotalsCalls = 0;
  mutable QDateTime lastFrom;
  mutable QDateTime lastTo;

  bool open() override { return true; }
  bool insertBatch(const QList<Activity> &) override { return true; }
  bool clearAll() override { return true; }

  QList<Activity> sessions(const QDateTime &, const QDateTime &,
                           int) const override {
    return {};
  }
  QList<AppTotal> appTotals(const QDateTime &,
                            const QDateTime &) const override {
    return {};
  }
  QList<TitleTotal> titleTotals(const QDateTime &from,
                                const QDateTime &to) const override {
    ++titleTotalsCalls;
    lastFrom = from;
    lastTo = to;
    return titles;
  }
  QList<Interval> intervals(const QDateTime &,
                            const QDateTime &) const override {
    return {};
  }
  RangeStats stats(const QDateTime &, const QDateTime &) const override {
    return {};
  }
  QPair<QDateTime, QDateTime> bounds() const override { return {}; }
  QStringList rankedAppNames(int) const override { return {}; }
};

const QDateTime kFrom(QDate(2026, 9, 1), QTime(0, 0), QTimeZone::UTC);
const QDateTime kTo(QDate(2026, 9, 2), QTime(0, 0), QTimeZone::UTC);

} // namespace

class TestActivityQueryService : public QObject {
  Q_OBJECT

private slots:
  void noRulesSkipsTheQuery();
  void categoriesFollowTheRules();
  void ruleChangeAppliesRetroactively();
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

QTEST_MAIN(TestActivityQueryService)
#include "tst_ActivityQueryService.moc"
