#include "core/AppSettings.hpp"
#include "ui/activity/ActivityFormat.hpp"
#include "ui/activity/CategoryRulesModel.hpp"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace chronexa::activity;
using chronexa::core::AppSettings;

class TestCategoryRulesModel : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();

  void seedsDefaultsOnFirstRun();
  void keepsAnEmptiedList();
  void editsAreSaved();
  void splitAndJoinApps();
  void moveRuleReorders_data();
  void moveRuleReorders();
  void colorSlotFollowsRuleOrder();
  void invalidRowsAreReported();
};

void TestCategoryRulesModel::initTestCase() {
  // Keep AppSettings' default QSettings away from the real Chronexa settings.
  QCoreApplication::setOrganizationName(QStringLiteral("ChronexaTests"));
  QCoreApplication::setApplicationName(QStringLiteral("tst_CategoryRules"));
}

void TestCategoryRulesModel::init() { QSettings().clear(); }

void TestCategoryRulesModel::cleanupTestCase() { QSettings().clear(); }

void TestCategoryRulesModel::seedsDefaultsOnFirstRun() {
  AppSettings settings;
  QVERIFY(!settings.hasCategoryRules());

  CategoryRulesModel model(&settings);

  QVERIFY(settings.hasCategoryRules());
  QCOMPARE(settings.categoryRules(), CategoryRulesModel::defaultRules());
  QCOMPARE(model.rowCount(), CategoryRulesModel::defaultRules().size());

  const CategoryRules rules(settings.categoryRules());
  QVERIFY2(!rules
                .categorize(QStringLiteral("Google Chrome"),
                            QStringLiteral("Cats - YouTube"))
                .isEmpty(),
           "YouTube in Chrome is claimed by a default rule");
  QVERIFY(!rules.categorize(QStringLiteral("CLion (GUI launcher)"), QString())
               .isEmpty());
  QCOMPARE(rules.categorize(QStringLiteral("Google Chrome"),
                            QStringLiteral("Pull Request")),
           QString());
}

void TestCategoryRulesModel::keepsAnEmptiedList() {
  {
    AppSettings settings;
    CategoryRulesModel model(&settings);
    while (model.rowCount() > 0) {
      model.removeRule(0);
    }
  }

  AppSettings reloaded;
  CategoryRulesModel model(&reloaded);
  QVERIFY2(model.rowCount() == 0,
           "a list emptied on purpose is not re-seeded on the next start");
}

void TestCategoryRulesModel::editsAreSaved() {
  AppSettings settings;
  CategoryRulesModel model(&settings);
  QSignalSpy changed(&settings, &AppSettings::categoryRulesChanged);

  model.addRule();
  const int row = model.rowCount() - 1;
  model.setCategory(row, QStringLiteral("  Reading "));
  model.setApps(row, QStringLiteral("Firefox, , Chrome ,"));
  model.setTitlePattern(row, QStringLiteral("wiki"));

  QCOMPARE(changed.count(), 4);

  AppSettings reloaded;
  const CategoryRule saved = reloaded.categoryRules().at(row);
  QCOMPARE(saved.category, QStringLiteral("Reading"));
  QCOMPARE(saved.apps,
           (QStringList{QStringLiteral("Firefox"), QStringLiteral("Chrome")}));
  QCOMPARE(saved.titlePattern, QStringLiteral("wiki"));

  model.setCategory(row, QStringLiteral("Reading"));
  QVERIFY2(changed.count() == 4, "an unchanged value is not saved again");
}

void TestCategoryRulesModel::splitAndJoinApps() {
  QCOMPARE(format::splitApps(QStringLiteral(" a ,b,, c ")),
           (QStringList{QStringLiteral("a"), QStringLiteral("b"),
                        QStringLiteral("c")}));
  QVERIFY(format::splitApps(QStringLiteral(" , ")).isEmpty());
  QCOMPARE(format::joinApps(
               {QStringLiteral("CLion"), QStringLiteral("Visual Studio Code")}),
           QStringLiteral("CLion, Visual Studio Code"));
}

void TestCategoryRulesModel::moveRuleReorders_data() {
  QTest::addColumn<int>("row");
  QTest::addColumn<int>("delta");
  QTest::addColumn<QStringList>("expected");

  QTest::newRow("down") << 0 << 1 << QStringList{"B", "A", "C"};
  QTest::newRow("up") << 2 << -1 << QStringList{"A", "C", "B"};
  QTest::newRow("past the top") << 0 << -1 << QStringList{"A", "B", "C"};
  QTest::newRow("past the bottom") << 2 << 1 << QStringList{"A", "B", "C"};
}

void TestCategoryRulesModel::moveRuleReorders() {
  QFETCH(int, row);
  QFETCH(int, delta);
  QFETCH(QStringList, expected);

  AppSettings settings;
  CategoryRulesModel model(&settings);
  while (model.rowCount() > 0) {
    model.removeRule(0);
  }
  for (const QString &name : {"A", "B", "C"}) {
    model.addRule();
    model.setCategory(model.rowCount() - 1, name);
    model.setApps(model.rowCount() - 1, name.toLower());
  }

  model.moveRule(row, delta);

  QStringList inModel;
  QStringList saved;
  for (int i = 0; i < model.rowCount(); ++i) {
    inModel.append(model.data(model.index(i), CategoryRulesModel::CategoryRole)
                       .toString());
    saved.append(settings.categoryRules().at(i).category);
  }
  QCOMPARE(inModel, expected);
  QCOMPARE(saved, expected);
}

void TestCategoryRulesModel::colorSlotFollowsRuleOrder() {
  AppSettings settings;
  CategoryRulesModel model(&settings);
  while (model.rowCount() > 0) {
    model.removeRule(0);
  }
  for (const QString &name : {"Work", "Chat", "Work"}) {
    model.addRule();
    model.setCategory(model.rowCount() - 1, name);
    model.setApps(model.rowCount() - 1, QStringLiteral("x"));
  }

  auto slot = [&model](int row) {
    return model.data(model.index(row), CategoryRulesModel::ColorSlotRole)
        .toInt();
  };
  QCOMPARE(slot(0), 0);
  QCOMPARE(slot(1), 1);
  QVERIFY2(slot(2) == 0, "two rules for one category share its colour");

  QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);
  model.moveRule(1, -1);
  QVERIFY(!dataChanged.isEmpty());
  QCOMPARE(slot(0), 0);
  QCOMPARE(
      model.data(model.index(0), CategoryRulesModel::CategoryRole).toString(),
      QStringLiteral("Chat"));
  QCOMPARE(slot(1), 1);
}

void TestCategoryRulesModel::invalidRowsAreReported() {
  AppSettings settings;
  CategoryRulesModel model(&settings);
  model.addRule();
  const int row = model.rowCount() - 1;
  const QModelIndex index = model.index(row);

  QVERIFY(!model.data(index, CategoryRulesModel::ValidRole).toBool());
  QCOMPARE(model.data(index, CategoryRulesModel::ColorSlotRole).toInt(), -1);

  model.setCategory(row, QStringLiteral("X"));
  model.setTitlePattern(row, QStringLiteral("(broken"));
  QVERIFY(!model.data(index, CategoryRulesModel::TitleValidRole).toBool());
  QVERIFY(!model.data(index, CategoryRulesModel::ValidRole).toBool());

  model.setTitlePattern(row, QStringLiteral("fixed"));
  QVERIFY(model.data(index, CategoryRulesModel::TitleValidRole).toBool());
  QVERIFY(model.data(index, CategoryRulesModel::ValidRole).toBool());
}

QTEST_MAIN(TestCategoryRulesModel)
#include "tst_CategoryRulesModel.moc"
