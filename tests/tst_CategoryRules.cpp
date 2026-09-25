#include "domain/activity/CategoryRules.hpp"

#include <QTest>

using namespace chronexa::activity;

namespace {

// The rule set from the feature request, in the order that makes it work: the
// narrow browser rule has to come before anything that would claim the
// browser as a whole.
CategoryRules sampleRules() {
  return CategoryRules({
      CategoryRule{QStringLiteral("Distractions"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
      CategoryRule{
          QStringLiteral("Work"),
          {QStringLiteral("CLion"), QStringLiteral("Visual Studio Code")},
          {}},
      CategoryRule{
          QStringLiteral("Communication"), {QStringLiteral("Slack")}, {}},
      CategoryRule{QStringLiteral("Reading"), {}, QStringLiteral("\\bwiki")},
  });
}

} // namespace

class TestCategoryRules : public QObject {
  Q_OBJECT

private slots:
  void categorize_data();
  void categorize();

  void firstMatchingRuleWins();
  void emptyRuleSetCategorizesNothing();

  void ruleValidity_data();
  void ruleValidity();
  void invalidRuleNeverMatches();

  void categoryNames();

  void serializationRoundTrip();
  void parseToleratesGarbage_data();
  void parseToleratesGarbage();

  void categoryTotalsAggregates();
  void categoryTotalsPreferManualCategory();
  void categoryTotalsRoundsOnce();
  void categoryTotalsWithoutRules();
};

void TestCategoryRules::categorize_data() {
  QTest::addColumn<QString>("app");
  QTest::addColumn<QString>("title");
  QTest::addColumn<QString>("expected");

  QTest::newRow("app substring, real exe description")
      << "CLion (GUI launcher)" << "main.cpp" << "Work";
  QTest::newRow("second app of the same rule")
      << "Visual Studio Code" << "README.md" << "Work";
  QTest::newRow("app match is case-insensitive") << "clion" << "" << "Work";
  QTest::newRow("app-only rule ignores the title")
      << "Slack" << "general | Ezlo" << "Communication";
  QTest::newRow("app and title both match")
      << "Google Chrome" << "Lo-fi beats - YouTube - Google Chrome"
      << "Distractions";
  QTest::newRow("title regex is case-insensitive")
      << "Google Chrome" << "youtube.com" << "Distractions";
  QTest::newRow("app matches, title does not")
      << "Google Chrome" << "Pull Request #3655 - Google Chrome" << "";
  QTest::newRow("title matches, app does not") << "Firefox" << "YouTube" << "";
  QTest::newRow("title-only rule applies to any app")
      << "Firefox" << "Qt - Wikipedia" << "Reading";
  QTest::newRow("title regex honours word boundaries")
      << "Firefox" << "kiwiki" << "";
  QTest::newRow("unknown app") << "Notepad" << "notes.txt" << "";
}

void TestCategoryRules::categorize() {
  QFETCH(QString, app);
  QFETCH(QString, title);
  QFETCH(QString, expected);

  QCOMPARE(sampleRules().categorize(app, title), expected);
}

void TestCategoryRules::firstMatchingRuleWins() {
  const CategoryRules browserFirst({
      CategoryRule{QStringLiteral("Browsing"), {QStringLiteral("Chrome")}, {}},
      CategoryRule{QStringLiteral("Distractions"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
  });
  QCOMPARE(browserFirst.categorize(QStringLiteral("Google Chrome"),
                                   QStringLiteral("YouTube")),
           QStringLiteral("Browsing"));

  const CategoryRules narrowFirst({
      CategoryRule{QStringLiteral("Distractions"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
      CategoryRule{QStringLiteral("Browsing"), {QStringLiteral("Chrome")}, {}},
  });
  QCOMPARE(narrowFirst.categorize(QStringLiteral("Google Chrome"),
                                  QStringLiteral("YouTube")),
           QStringLiteral("Distractions"));
  QCOMPARE(narrowFirst.categorize(QStringLiteral("Google Chrome"),
                                  QStringLiteral("Docs")),
           QStringLiteral("Browsing"));
}

void TestCategoryRules::emptyRuleSetCategorizesNothing() {
  const CategoryRules rules;
  QVERIFY(rules.isEmpty());
  QCOMPARE(rules.categorize(QStringLiteral("CLion"), QStringLiteral("x")),
           QString());
}

void TestCategoryRules::ruleValidity_data() {
  QTest::addColumn<QString>("category");
  QTest::addColumn<QStringList>("apps");
  QTest::addColumn<QString>("titlePattern");
  QTest::addColumn<bool>("valid");

  QTest::newRow("apps only") << "Work" << QStringList{"CLion"} << "" << true;
  QTest::newRow("title only") << "Work" << QStringList{} << "jira" << true;
  QTest::newRow("both") << "Work" << QStringList{"Chrome"} << "jira" << true;
  QTest::newRow("no category") << "" << QStringList{"CLion"} << "" << false;
  QTest::newRow("blank category")
      << "   " << QStringList{"CLion"} << "" << false;
  QTest::newRow("matches everything") << "Work" << QStringList{} << "" << false;
  QTest::newRow("blank app entries only")
      << "Work" << QStringList{" ", ""} << "" << false;
  QTest::newRow("broken regex")
      << "Work" << QStringList{"Chrome"} << "(unclosed" << false;
}

void TestCategoryRules::ruleValidity() {
  QFETCH(QString, category);
  QFETCH(QStringList, apps);
  QFETCH(QString, titlePattern);
  QFETCH(bool, valid);

  const CategoryRule rule{category, apps, titlePattern};
  QCOMPARE(rule.isValid(), valid);
}

void TestCategoryRules::invalidRuleNeverMatches() {
  const CategoryRules rules({
      CategoryRule{QStringLiteral("Broken"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("(unclosed")},
      CategoryRule{QString(), {QStringLiteral("Chrome")}, {}},
      CategoryRule{QStringLiteral("Everything"), {}, {}},
      CategoryRule{QStringLiteral("Fallback"), {QStringLiteral("Chrome")}, {}},
  });
  QVERIFY2(!rules.isEmpty(), "rules are kept even when some are unusable");
  QCOMPARE(rules.categorize(QStringLiteral("Google Chrome"),
                            QStringLiteral("(unclosed")),
           QStringLiteral("Fallback"));
  QCOMPARE(rules.categorize(QStringLiteral("Notepad"), QString()), QString());
}

void TestCategoryRules::categoryNames() {
  const CategoryRules rules({
      CategoryRule{QStringLiteral("Work"), {QStringLiteral("CLion")}, {}},
      CategoryRule{QStringLiteral("Chat"), {QStringLiteral("Slack")}, {}},
      CategoryRule{QStringLiteral(" Work "), {QStringLiteral("Code")}, {}},
      CategoryRule{QStringLiteral("Broken"), {}, QStringLiteral("(")},
  });
  QCOMPARE(rules.categoryNames(),
           (QStringList{QStringLiteral("Work"), QStringLiteral("Chat")}));
}

void TestCategoryRules::serializationRoundTrip() {
  const QList<CategoryRule> rules = sampleRules().rules();
  const QString json = serializeCategoryRules(rules);
  QCOMPARE(parseCategoryRules(json), rules);
}

void TestCategoryRules::parseToleratesGarbage_data() {
  QTest::addColumn<QString>("json");
  QTest::addColumn<int>("count");

  QTest::newRow("empty string") << "" << 0;
  QTest::newRow("not json") << "{{{" << 0;
  QTest::newRow("object instead of array") << "{\"category\":\"W\"}" << 0;
  QTest::newRow("non-object entries skipped")
      << "[1, \"x\", {\"category\":\"W\",\"apps\":[\"a\"]}]" << 1;
  QTest::newRow("missing fields become empty") << "[{\"category\":\"W\"}]" << 1;
}

void TestCategoryRules::parseToleratesGarbage() {
  QFETCH(QString, json);
  QFETCH(int, count);

  QCOMPARE(parseCategoryRules(json).size(), count);
}

void TestCategoryRules::categoryTotalsAggregates() {
  const QList<TitleTotal> titles = {
      TitleTotal{QStringLiteral("CLion (GUI launcher)"),
                 QStringLiteral("a.cpp"), 60'000, 2},
      TitleTotal{QStringLiteral("Google Chrome"), QStringLiteral("YouTube"),
                 90'000, 3},
      TitleTotal{QStringLiteral("Visual Studio Code"), QStringLiteral("b.py"),
                 45'000, 1},
      TitleTotal{QStringLiteral("Google Chrome"), QStringLiteral("PR #1"),
                 500'000, 4},
      TitleTotal{QStringLiteral("Slack"), QStringLiteral("general"), 30'000, 1},
  };

  const QList<CategoryTotal> totals = categoryTotals(titles, sampleRules());

  QCOMPARE(totals.size(), 4);

  QCOMPARE(totals.at(0).category, QStringLiteral("Work"));
  QCOMPARE(totals.at(0).seconds, 105);
  QCOMPARE(totals.at(0).sessionCount, 3);

  QCOMPARE(totals.at(1).category, QStringLiteral("Distractions"));
  QCOMPARE(totals.at(1).seconds, 90);
  QCOMPARE(totals.at(1).sessionCount, 3);

  QCOMPARE(totals.at(2).category, QStringLiteral("Communication"));
  QCOMPARE(totals.at(2).seconds, 30);

  QVERIFY2(totals.at(3).category.isEmpty(),
           "the uncategorized rest comes last even when it is the largest");
  QCOMPARE(totals.at(3).seconds, 500);
  QCOMPARE(totals.at(3).sessionCount, 4);
}

void TestCategoryRules::categoryTotalsPreferManualCategory() {
  TitleTotal learning{QStringLiteral("Google Chrome"),
                      QStringLiteral("YouTube"), 60'000, 1};
  learning.category = QStringLiteral("Learning");
  TitleTotal none{QStringLiteral("CLion"), QStringLiteral("a.cpp"), 30'000, 1};
  none.category = QStringLiteral("");
  const QList<TitleTotal> titles = {
      learning,
      none,
      TitleTotal{QStringLiteral("Google Chrome"), QStringLiteral("YouTube"),
                 20'000, 1},
  };

  const QList<CategoryTotal> totals = categoryTotals(titles, sampleRules());

  QCOMPARE(totals.size(), 3);
  QCOMPARE(totals.at(0).category, QStringLiteral("Learning"));
  QCOMPARE(totals.at(0).seconds, 60);
  QVERIFY2(totals.at(1).category == QStringLiteral("Distractions") &&
               totals.at(1).seconds == 20,
           "the rest of the same window still follows the rules");
  QVERIFY2(totals.at(2).category.isEmpty() && totals.at(2).seconds == 30,
           "'no category' by hand beats the CLion -> Work rule");
}

void TestCategoryRules::categoryTotalsRoundsOnce() {
  const QList<TitleTotal> titles = {
      TitleTotal{QStringLiteral("CLion"), QStringLiteral("a"), 1'600, 1},
      TitleTotal{QStringLiteral("CLion"), QStringLiteral("b"), 1'600, 1},
  };
  const QList<CategoryTotal> totals = categoryTotals(titles, sampleRules());
  QCOMPARE(totals.size(), 1);
  QVERIFY2(totals.at(0).seconds == 3,
           "3.2 s summed in ms, not 1 s + 1 s rounded per title");
}

void TestCategoryRules::categoryTotalsWithoutRules() {
  const QList<TitleTotal> titles = {
      TitleTotal{QStringLiteral("CLion"), QStringLiteral("a"), 10'000, 1},
      TitleTotal{QStringLiteral("Slack"), QStringLiteral("b"), 5'000, 1},
  };
  const QList<CategoryTotal> totals = categoryTotals(titles, CategoryRules());
  QCOMPARE(totals.size(), 1);
  QVERIFY(totals.at(0).category.isEmpty());
  QCOMPARE(totals.at(0).seconds, 15);
  QCOMPARE(totals.at(0).sessionCount, 2);

  QVERIFY(categoryTotals({}, sampleRules()).isEmpty());
}

QTEST_MAIN(TestCategoryRules)
#include "tst_CategoryRules.moc"
