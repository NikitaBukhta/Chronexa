#include "domain/activity/PrivacyRules.hpp"

#include <QTest>

using namespace chronexa::activity;

Q_DECLARE_METATYPE(chronexa::activity::Privacy)

namespace {

PrivacyRules sampleRules() {
  return PrivacyRules({
      PrivacyRule{Privacy::Exclude, {QStringLiteral("KeePass")}, {}},
      PrivacyRule{Privacy::Exclude,
                  {QStringLiteral("Chrome")},
                  QStringLiteral("Incognito")},
      PrivacyRule{Privacy::HideTitle,
                  {QStringLiteral("Telegram"), QStringLiteral("Chrome")},
                  {}},
      PrivacyRule{Privacy::HideTitle, {}, QStringLiteral("\\bbank\\b")},
  });
}

} // namespace

class TestPrivacyRules : public QObject {
  Q_OBJECT

private slots:
  void classify_data();
  void classify();

  void strictestRuleWinsWhateverTheOrder();
  void emptyRuleSetRecordsEverything();

  void ruleValidity_data();
  void ruleValidity();
  void invalidRuleNeverApplies();

  void keysRoundTrip();
  void serializationRoundTrip();
  void parseToleratesGarbage();
};

void TestPrivacyRules::classify_data() {
  QTest::addColumn<QString>("app");
  QTest::addColumn<QString>("title");
  QTest::addColumn<Privacy>("expected");

  QTest::newRow("excluded app, substring of the description")
      << "KeePassXC" << "Passwords.kdbx" << Privacy::Exclude;
  QTest::newRow("app match is case-insensitive")
      << "keepass password safe" << "" << Privacy::Exclude;
  QTest::newRow("hidden title app")
      << "Telegram Desktop" << "Olena: see you at 7" << Privacy::HideTitle;
  QTest::newRow("exclusion by title beats hiding the same app")
      << "Google Chrome" << "New Tab - Google Chrome (Incognito)"
      << Privacy::Exclude;
  QTest::newRow("same app, ordinary title: only hidden")
      << "Google Chrome" << "Pull Request #3" << Privacy::HideTitle;
  QTest::newRow("title-only rule applies to any app")
      << "Firefox" << "My Bank - Accounts" << Privacy::HideTitle;
  QTest::newRow("title regex honours word boundaries")
      << "Firefox" << "Riverbank walk" << Privacy::Record;
  QTest::newRow("unmatched window is recorded")
      << "CLion" << "main.cpp" << Privacy::Record;
}

void TestPrivacyRules::classify() {
  QFETCH(QString, app);
  QFETCH(QString, title);
  QFETCH(Privacy, expected);

  QCOMPARE(sampleRules().classify(app, title), expected);
}

void TestPrivacyRules::strictestRuleWinsWhateverTheOrder() {
  const PrivacyRule hide{Privacy::HideTitle, {QStringLiteral("Signal")}, {}};
  const PrivacyRule exclude{Privacy::Exclude, {QStringLiteral("Signal")}, {}};

  QCOMPARE(PrivacyRules({hide, exclude})
               .classify(QStringLiteral("Signal"), QStringLiteral("chat")),
           Privacy::Exclude);
  QCOMPARE(PrivacyRules({exclude, hide})
               .classify(QStringLiteral("Signal"), QStringLiteral("chat")),
           Privacy::Exclude);
}

void TestPrivacyRules::emptyRuleSetRecordsEverything() {
  const PrivacyRules rules;
  QVERIFY(rules.isEmpty());
  QCOMPARE(rules.classify(QStringLiteral("KeePass"), QStringLiteral("x")),
           Privacy::Record);
}

void TestPrivacyRules::ruleValidity_data() {
  QTest::addColumn<int>("action");
  QTest::addColumn<QStringList>("apps");
  QTest::addColumn<QString>("title");
  QTest::addColumn<bool>("valid");
  QTest::addColumn<bool>("titleValid");

  const int exclude = static_cast<int>(Privacy::Exclude);
  QTest::newRow("app only") << exclude << QStringList{"KeePass"} << ""
                            << true << true;
  QTest::newRow("title only") << exclude << QStringList{} << "bank" << true
                              << true;
  QTest::newRow("no condition would match every window")
      << exclude << QStringList{} << "" << false << true;
  QTest::newRow("blank apps and title count as none")
      << exclude << QStringList{"  ", ""} << "  " << false << true;
  QTest::newRow("broken regex")
      << exclude << QStringList{"Chrome"} << "(" << false << false;
  QTest::newRow("Record does nothing, so it is not a rule")
      << static_cast<int>(Privacy::Record) << QStringList{"KeePass"} << ""
      << false << true;
}

void TestPrivacyRules::ruleValidity() {
  QFETCH(int, action);
  QFETCH(QStringList, apps);
  QFETCH(QString, title);
  QFETCH(bool, valid);
  QFETCH(bool, titleValid);

  const PrivacyRule rule{static_cast<Privacy>(action), apps, title};
  QCOMPARE(rule.isValid(), valid);
  const PrivacyRules rules({rule});
  QCOMPARE(rules.isUsable(0), valid);
  QCOMPARE(rules.hasValidTitle(0), titleValid);
  QVERIFY(!rules.isUsable(1));
}

void TestPrivacyRules::invalidRuleNeverApplies() {
  const PrivacyRules rules({
      PrivacyRule{Privacy::Exclude, {}, {}},
      PrivacyRule{Privacy::Exclude, {QStringLiteral("Chrome")},
                  QStringLiteral("(")},
  });
  QCOMPARE(rules.classify(QStringLiteral("Google Chrome"), QStringLiteral("(")),
           Privacy::Record);
}

void TestPrivacyRules::keysRoundTrip() {
  for (const Privacy action : {Privacy::Exclude, Privacy::HideTitle}) {
    QCOMPARE(privacyFromKey(privacyKey(action)), action);
  }
  QCOMPARE(privacyKey(Privacy::Exclude), QStringLiteral("exclude"));
  QCOMPARE(privacyKey(Privacy::HideTitle), QStringLiteral("hideTitle"));
  QCOMPARE(privacyFromKey(QStringLiteral("bogus")), Privacy::Record);
}

void TestPrivacyRules::serializationRoundTrip() {
  const QList<PrivacyRule> rules = sampleRules().rules();
  const QString json = serializePrivacyRules(rules);
  QCOMPARE(parsePrivacyRules(json), rules);
}

void TestPrivacyRules::parseToleratesGarbage() {
  QVERIFY(parsePrivacyRules(QString()).isEmpty());
  QVERIFY(parsePrivacyRules(QStringLiteral("not json")).isEmpty());
  QVERIFY(parsePrivacyRules(QStringLiteral("{\"a\":1}")).isEmpty());

  const QList<PrivacyRule> rules = parsePrivacyRules(
      QStringLiteral("[1, {\"action\":\"nope\",\"apps\":[\"X\"]},"
                     "{\"action\":\"exclude\",\"apps\":[\"KeePass\"]}]"));
  QCOMPARE(rules.size(), 2);
  QVERIFY2(!rules.at(0).isValid(),
           "an unknown action is kept but never applied");
  QCOMPARE(rules.at(1).action, Privacy::Exclude);
  QVERIFY(rules.at(1).isValid());
}

QTEST_MAIN(TestPrivacyRules)
#include "tst_PrivacyRules.moc"
