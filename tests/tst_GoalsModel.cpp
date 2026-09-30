#include "core/AppSettings.hpp"
#include "ui/activity/CategoryRulesModel.hpp"
#include "ui/activity/GoalsModel.hpp"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace chronexa::activity;
using chronexa::core::AppSettings;

namespace {

QVariant role(const GoalsModel &model, int row, GoalsModel::Role role) {
  return model.data(model.index(row), role);
}

} // namespace

class TestGoalsModel : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();

  void seedsDefaultsForTheDefaultCategories();
  void seedsOnlyCategoriesThatExist();
  void keepsAnEmptiedList();
  void editsAreSaved();
  void minutesAreClamped();
  void addGoalPicksAnUnusedCategory();
  void renamedCategoryIsFlagged();
  void removeGoal();
};

void TestGoalsModel::initTestCase() {
  QCoreApplication::setOrganizationName(QStringLiteral("ChronexaTests"));
  QCoreApplication::setApplicationName(QStringLiteral("tst_GoalsModel"));
}

void TestGoalsModel::init() { QSettings().clear(); }

void TestGoalsModel::cleanupTestCase() { QSettings().clear(); }

void TestGoalsModel::seedsDefaultsForTheDefaultCategories() {
  AppSettings settings;
  CategoryRulesModel rules(&settings);
  QVERIFY(!settings.hasDailyGoals());

  GoalsModel model(&settings);

  QVERIFY(settings.hasDailyGoals());
  const QList<CategoryRule> defaults = CategoryRulesModel::defaultRules();
  QCOMPARE(settings.dailyGoals(),
           (QList<DailyGoal>{
               {defaults[0].category, GoalKind::Limit, 60,
                TrackingSchedule::kEveryDay},
               {defaults[1].category, GoalKind::Target, 240,
                TrackingSchedule::kWorkdays},
           }));
  QCOMPARE(model.rowCount(), 2);
  QCOMPARE(role(model, 0, GoalsModel::KindRole).toString(),
           QStringLiteral("limit"));
  QCOMPARE(role(model, 1, GoalsModel::KindRole).toString(),
           QStringLiteral("target"));
  QVERIFY(role(model, 0, GoalsModel::KnownCategoryRole).toBool());
  QCOMPARE(role(model, 1, GoalsModel::ColorSlotRole).toInt(), 1);
}

void TestGoalsModel::seedsOnlyCategoriesThatExist() {
  // An install from before goals, with rules of the user's own.
  AppSettings settings;
  settings.setCategoryRules(
      {CategoryRule{QStringLiteral("Deep work"), {QStringLiteral("CLion")},
                    {}},
       CategoryRulesModel::defaultRules()[0]});

  GoalsModel model(&settings);

  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(settings.dailyGoals()[0].category,
           CategoryRulesModel::defaultRules()[0].category);
}

void TestGoalsModel::keepsAnEmptiedList() {
  {
    AppSettings settings;
    CategoryRulesModel rules(&settings);
    GoalsModel model(&settings);
    model.removeGoal(1);
    model.removeGoal(0);
    QCOMPARE(model.rowCount(), 0);
  }
  AppSettings reloaded;
  GoalsModel model(&reloaded);
  QVERIFY2(model.rowCount() == 0, "an emptied list is not re-seeded");
}

void TestGoalsModel::editsAreSaved() {
  AppSettings settings;
  settings.setCategoryRules(CategoryRulesModel::defaultRules());
  settings.setDailyGoals({});
  GoalsModel model(&settings);
  model.addGoal();
  QSignalSpy saved(&settings, &AppSettings::dailyGoalsChanged);
  QSignalSpy changed(&model, &QAbstractItemModel::dataChanged);

  model.setKind(0, QStringLiteral("target"));
  model.setMinutes(0, 150);
  model.toggleDay(0, 6);
  model.toggleDay(0, 7);
  model.setCategory(0, QStringLiteral("  Work "));

  QCOMPARE(saved.size(), 5);
  QCOMPARE(changed.size(), 5);
  const DailyGoal expected{QStringLiteral("Work"), GoalKind::Target, 150,
                           TrackingSchedule::kWorkdays};
  QCOMPARE(settings.dailyGoals()[0], expected);

  AppSettings reloaded;
  QCOMPARE(reloaded.dailyGoals()[0], expected);

  model.setMinutes(0, 150);
  model.toggleDay(0, 0);
  model.toggleDay(0, 8);
  QVERIFY2(saved.size() == 5, "no-op edits are not saved");
}

void TestGoalsModel::minutesAreClamped() {
  AppSettings settings;
  settings.setDailyGoals({{QStringLiteral("Work"), GoalKind::Limit, 60,
                           TrackingSchedule::kEveryDay}});
  GoalsModel model(&settings);

  model.setMinutes(0, 5000);
  QCOMPARE(settings.dailyGoals()[0].minutes, DailyGoal::kMaxMinutes);
  model.setMinutes(0, -3);
  QCOMPARE(settings.dailyGoals()[0].minutes, 0);
  QVERIFY(!role(model, 0, GoalsModel::ValidRole).toBool());
}

void TestGoalsModel::addGoalPicksAnUnusedCategory() {
  AppSettings settings;
  CategoryRulesModel rules(&settings);
  GoalsModel model(&settings);
  QSignalSpy count(&model, &GoalsModel::countChanged);

  model.addGoal();

  QCOMPARE(count.size(), 1);
  QCOMPARE(model.rowCount(), 3);
  QVERIFY2(role(model, 2, GoalsModel::CategoryRole).toString() ==
               CategoryRulesModel::defaultRules()[2].category,
           "the two default goals took the first two categories");
  QCOMPARE(settings.dailyGoals().size(), 3);
}

void TestGoalsModel::renamedCategoryIsFlagged() {
  AppSettings settings;
  CategoryRulesModel rules(&settings);
  GoalsModel model(&settings);
  QSignalSpy names(&model, &GoalsModel::categoryNamesChanged);
  QVERIFY(role(model, 0, GoalsModel::KnownCategoryRole).toBool());

  rules.setCategory(0, QStringLiteral("Time sinks"));

  QCOMPARE(names.size(), 1);
  QVERIFY(model.categoryNames().contains(QStringLiteral("Time sinks")));
  QVERIFY(!role(model, 0, GoalsModel::KnownCategoryRole).toBool());
  QCOMPARE(role(model, 0, GoalsModel::ColorSlotRole).toInt(), -1);
  QVERIFY2(role(model, 0, GoalsModel::ValidRole).toBool(),
           "the goal is kept: the user may rename it back or pick another");
}

void TestGoalsModel::removeGoal() {
  AppSettings settings;
  CategoryRulesModel rules(&settings);
  GoalsModel model(&settings);

  model.removeGoal(7);
  QCOMPARE(model.rowCount(), 2);

  model.removeGoal(0);
  QCOMPARE(model.rowCount(), 1);
  QCOMPARE(settings.dailyGoals()[0].kind, GoalKind::Target);
}

QTEST_GUILESS_MAIN(TestGoalsModel)
#include "tst_GoalsModel.moc"
