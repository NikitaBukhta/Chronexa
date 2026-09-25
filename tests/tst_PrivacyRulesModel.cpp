#include "core/AppSettings.hpp"
#include "ui/activity/PrivacyRulesModel.hpp"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>

using namespace chronexa::activity;
using chronexa::core::AppSettings;

class TestPrivacyRulesModel : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanupTestCase();

  void seedsDefaultsOnFirstRun();
  void keepsAnEmptiedList();
  void editsAreSaved();
  void unknownActionIsIgnored();
  void invalidRowsAreReported();
  void restoreDefaults();
};

void TestPrivacyRulesModel::initTestCase() {
  // Keep AppSettings' default QSettings away from the real Chronexa settings.
  QCoreApplication::setOrganizationName(QStringLiteral("ChronexaTests"));
  QCoreApplication::setApplicationName(QStringLiteral("tst_PrivacyRules"));
}

void TestPrivacyRulesModel::init() { QSettings().clear(); }

void TestPrivacyRulesModel::cleanupTestCase() { QSettings().clear(); }

void TestPrivacyRulesModel::seedsDefaultsOnFirstRun() {
  AppSettings settings;
  QVERIFY(!settings.hasPrivacyRules());

  PrivacyRulesModel model(&settings);

  QVERIFY(settings.hasPrivacyRules());
  QCOMPARE(settings.privacyRules(), PrivacyRulesModel::defaultRules());
  QCOMPARE(model.rowCount(), PrivacyRulesModel::defaultRules().size());

  const PrivacyRules rules(settings.privacyRules());
  for (int i = 0; i < rules.rules().size(); ++i) {
    QVERIFY2(rules.isUsable(i), "every default rule is usable");
  }
  QCOMPARE(rules.classify(QStringLiteral("KeePassXC"), QString()),
           Privacy::Exclude);
  QCOMPARE(rules.classify(QStringLiteral("1Password"), QString()),
           Privacy::Exclude);
  QCOMPARE(rules.classify(QStringLiteral("Firefox"),
                          QStringLiteral("Mozilla Firefox Private Browsing")),
           Privacy::Exclude);
  QCOMPARE(rules.classify(QStringLiteral("Firefox"),
                          QStringLiteral("Новая вкладка — Приватный просмотр")),
           Privacy::Exclude);
  QCOMPARE(rules.classify(QStringLiteral("Firefox"),
                          QStringLiteral("Нова вкладка — Приватний перегляд")),
           Privacy::Exclude);
  QCOMPARE(rules.classify(QStringLiteral("Microsoft Edge"),
                          QStringLiteral("News and 1 more page - [InPrivate]")),
           Privacy::Exclude);
  QCOMPARE(rules.classify(QStringLiteral("Telegram Desktop"),
                          QStringLiteral("Olena")),
           Privacy::HideTitle);
  QCOMPARE(rules.classify(QStringLiteral("Google Chrome"),
                          QStringLiteral("Pull Request")),
           Privacy::Record);
  QCOMPARE(rules.classify(QStringLiteral("CLion"), QStringLiteral("main.cpp")),
           Privacy::Record);
}

void TestPrivacyRulesModel::keepsAnEmptiedList() {
  {
    AppSettings settings;
    PrivacyRulesModel model(&settings);
    while (model.rowCount() > 0) {
      model.removeRule(0);
    }
  }

  AppSettings reloaded;
  PrivacyRulesModel model(&reloaded);
  QVERIFY2(model.rowCount() == 0,
           "a list emptied on purpose is not re-seeded on the next start");
}

void TestPrivacyRulesModel::editsAreSaved() {
  AppSettings settings;
  PrivacyRulesModel model(&settings);
  QSignalSpy changed(&settings, &AppSettings::privacyRulesChanged);
  QSignalSpy count(&model, &PrivacyRulesModel::countChanged);

  model.addRule();
  QCOMPARE(count.count(), 1);
  const int row = model.rowCount() - 1;
  QCOMPARE(model.data(model.index(row), PrivacyRulesModel::ActionRole),
           QVariant(QStringLiteral("hideTitle")));

  QSignalSpy dataChanged(&model, &QAbstractItemModel::dataChanged);
  model.setAction(row, QStringLiteral("exclude"));
  model.setApps(row, QStringLiteral("Signal, , Viber ,"));
  model.setTitlePattern(row, QStringLiteral("  secret  "));
  QCOMPARE(changed.count(), 4);
  QCOMPARE(dataChanged.count(), 3);

  AppSettings reloaded;
  const PrivacyRule saved = reloaded.privacyRules().at(row);
  QCOMPARE(saved.action, Privacy::Exclude);
  QCOMPARE(saved.apps,
           (QStringList{QStringLiteral("Signal"), QStringLiteral("Viber")}));
  QCOMPARE(saved.titlePattern, QStringLiteral("secret"));
  QCOMPARE(model.data(model.index(row), PrivacyRulesModel::AppsRole),
           QVariant(QStringLiteral("Signal, Viber")));

  model.setAction(row, QStringLiteral("exclude"));
  QVERIFY2(changed.count() == 4, "an unchanged value is not saved again");
}

void TestPrivacyRulesModel::unknownActionIsIgnored() {
  AppSettings settings;
  PrivacyRulesModel model(&settings);
  const QList<PrivacyRule> before = settings.privacyRules();
  QSignalSpy changed(&settings, &AppSettings::privacyRulesChanged);

  model.setAction(0, QStringLiteral("record"));
  model.setAction(0, QString());
  model.setAction(99, QStringLiteral("exclude"));

  QVERIFY(changed.isEmpty());
  QCOMPARE(settings.privacyRules(), before);
}

void TestPrivacyRulesModel::invalidRowsAreReported() {
  AppSettings settings;
  PrivacyRulesModel model(&settings);
  model.addRule();
  const QModelIndex row = model.index(model.rowCount() - 1);

  QVERIFY2(!model.data(row, PrivacyRulesModel::ValidRole).toBool(),
           "a new rule has no condition yet");
  QVERIFY(model.data(row, PrivacyRulesModel::TitleValidRole).toBool());

  model.setTitlePattern(row.row(), QStringLiteral("("));
  QVERIFY(!model.data(row, PrivacyRulesModel::TitleValidRole).toBool());
  QVERIFY(!model.data(row, PrivacyRulesModel::ValidRole).toBool());

  model.setTitlePattern(row.row(), QStringLiteral("bank"));
  QVERIFY(model.data(row, PrivacyRulesModel::TitleValidRole).toBool());
  QVERIFY(model.data(row, PrivacyRulesModel::ValidRole).toBool());
}

void TestPrivacyRulesModel::restoreDefaults() {
  AppSettings settings;
  PrivacyRulesModel model(&settings);
  while (model.rowCount() > 0) {
    model.removeRule(0);
  }
  QVERIFY(settings.privacyRules().isEmpty());

  model.restoreDefaults();
  QCOMPARE(model.rowCount(), PrivacyRulesModel::defaultRules().size());
  QCOMPARE(settings.privacyRules(), PrivacyRulesModel::defaultRules());
}

QTEST_GUILESS_MAIN(TestPrivacyRulesModel)
#include "tst_PrivacyRulesModel.moc"
