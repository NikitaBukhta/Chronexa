#include "application/activity/ActivityService.hpp"
#include "fakes/ForegroundScript.hpp"

#include <QSignalSpy>
#include <QTest>

#include <memory>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const QList<CategoryRule> kRules = {
    CategoryRule{QStringLiteral("Distractions"),
                 {QStringLiteral("Chrome")},
                 QStringLiteral("YouTube")},
    CategoryRule{QStringLiteral("Work"), {QStringLiteral("CLion")}, {}},
};

const QList<PrivacyRule> kPrivacy = {
    PrivacyRule{Privacy::Exclude, {QStringLiteral("KeePass")}, {}},
    PrivacyRule{Privacy::HideTitle, {QStringLiteral("Telegram")}, {}},
};

Activity session(const QString &app, int fromMinute, int toMinute) {
  Activity activity;
  activity.appName = app;
  activity.title = app;
  activity.startedOn = utc(10, fromMinute);
  activity.endedOn = utc(10, toMinute);
  return activity;
}

} // namespace

class TestActivityService : public QObject {
  Q_OBJECT

private slots:
  void startsTheProviderWhenTrackingIsOn();
  void categoryRulesBecomeTheSessionKey();
  void stoppingTrackingFlushesTheProvider();
  void failedInsertIsRetried();
  void clearDropsWhatIsBuffered();
  void browserTabSwitchIsRecordedAsTwoSessions();

  void privacyRulesBecomeTheProviderFilter();
  void sessionsSampledBeforeTheRulesAreRedactedOnFlush();
  void applyPrivacyToHistoryRedactsWhatIsStored();
  void applyPrivacyToHistoryReportsFailure();
  void privateWindowsNeverReachTheRepository();
};

void TestActivityService::startsTheProviderWhenTrackingIsOn() {
  FakeActivityRepository repository;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();

  ActivityService service(repository, std::move(provider));

  QCOMPARE(mock->startCalls, 1);
  QVERIFY(service.isTracking());
}

void TestActivityService::categoryRulesBecomeTheSessionKey() {
  FakeActivityRepository repository;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));

  QVERIFY(!mock->sessionKey);
  service.setCategoryRules(kRules);
  QVERIFY(mock->sessionKey);

  QCOMPARE(mock->sessionKey(QStringLiteral("Google Chrome"),
                            QStringLiteral("Cats - YouTube")),
           QStringLiteral("Distractions"));
  QCOMPARE(
      mock->sessionKey(QStringLiteral("Google Chrome"), QStringLiteral("Jira")),
      QString());
  QCOMPARE(mock->sessionKey(QStringLiteral("CLion (GUI launcher)"),
                            QStringLiteral("main.cpp")),
           QStringLiteral("Work"));

  service.setCategoryRules({});
  QVERIFY2(mock->sessionKey(QStringLiteral("CLion"), QString()).isEmpty(),
           "clearing the rules also clears the split key");
}

void TestActivityService::stoppingTrackingFlushesTheProvider() {
  FakeActivityRepository repository;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);

  mock->pendingEvents = {session(QStringLiteral("CLion"), 0, 5)};
  service.setTrackingEnabled(false);

  QCOMPARE(mock->stopCalls, 1);
  QCOMPARE(repository.inserted.size(), 1);
  QCOMPARE(recorded.count(), 1);
}

void TestActivityService::failedInsertIsRetried() {
  FakeActivityRepository repository;
  repository.failInserts = true;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));

  mock->pendingEvents = {session(QStringLiteral("CLion"), 0, 5)};
  service.setTrackingEnabled(false);
  QVERIFY(repository.inserted.isEmpty());

  repository.failInserts = false;
  mock->pendingEvents = {session(QStringLiteral("Slack"), 5, 6)};
  service.setTrackingEnabled(true);
  service.setTrackingEnabled(false);

  QVERIFY2(repository.inserted.size() == 2,
           "the session refused earlier is written with the next batch");
  QCOMPARE(repository.inserted.at(0).appName, QStringLiteral("CLion"));
}

void TestActivityService::clearDropsWhatIsBuffered() {
  FakeActivityRepository repository;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));
  QSignalSpy cleared(&service, &ActivityService::cleared);

  mock->pendingEvents = {session(QStringLiteral("CLion"), 0, 5)};
  service.requestClear();
  service.setTrackingEnabled(false);

  QCOMPARE(repository.clearCalls, 1);
  QCOMPARE(cleared.count(), 1);
  QVERIFY2(repository.inserted.isEmpty(),
           "sessions from before the clear are not written after it");
}

// The whole write path with only the OS mocked: rules set on the service
// reach the real polling provider, and the repository receives one session
// per category.
void TestActivityService::browserTabSwitchIsRecordedAsTwoSessions() {
  FakeActivityRepository repository;
  ForegroundScript script;
  ActivityService service(repository, script.takeProvider());
  service.setCategoryRules(kRules);

  const QString chrome = QStringLiteral("Google Chrome");
  script.hold(chrome, QStringLiteral("Jira - EPPT-9070"), 6);
  script.hold(chrome, QStringLiteral("Lo-fi beats - YouTube"), 9);
  script.leave();

  service.setTrackingEnabled(false);

  QCOMPARE(repository.inserted.size(), 2);
  QCOMPARE(repository.inserted.at(0).title, QStringLiteral("Jira - EPPT-9070"));
  QCOMPARE(repository.inserted.at(0).durationSeconds(), 6);
  QCOMPARE(repository.inserted.at(1).title,
           QStringLiteral("Lo-fi beats - YouTube"));
  QCOMPARE(repository.inserted.at(1).durationSeconds(), 9);

  const CategoryRules rules(kRules);
  QCOMPARE(rules.categorize(repository.inserted.at(0).appName,
                            repository.inserted.at(0).title),
           QString());
  QCOMPARE(rules.categorize(repository.inserted.at(1).appName,
                            repository.inserted.at(1).title),
           QStringLiteral("Distractions"));
}

void TestActivityService::privacyRulesBecomeTheProviderFilter() {
  FakeActivityRepository repository;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));

  QVERIFY(!mock->privacyFilter);
  service.setPrivacyRules(kPrivacy);
  QVERIFY(mock->privacyFilter);

  QCOMPARE(mock->privacyFilter(QStringLiteral("KeePassXC"), QString()),
           Privacy::Exclude);
  QCOMPARE(mock->privacyFilter(QStringLiteral("Telegram Desktop"),
                               QStringLiteral("Olena")),
           Privacy::HideTitle);
  QCOMPARE(mock->privacyFilter(QStringLiteral("CLion"), QString()),
           Privacy::Record);

  service.setPrivacyRules({});
  QCOMPARE(mock->privacyFilter(QStringLiteral("KeePassXC"), QString()),
           Privacy::Record);
}

void TestActivityService::sessionsSampledBeforeTheRulesAreRedactedOnFlush() {
  FakeActivityRepository repository;
  repository.failInserts = true;
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));

  // Held back by a failed insert, from before any rule existed.
  mock->pendingEvents = {session(QStringLiteral("Telegram"), 0, 5),
                         session(QStringLiteral("KeePass"), 5, 6)};
  service.setTrackingEnabled(false);
  QVERIFY(repository.inserted.isEmpty());

  service.setPrivacyRules(kPrivacy);
  repository.failInserts = false;
  // Drained after the rules arrived, but sampled before the filter was in
  // place.
  mock->pendingEvents = {session(QStringLiteral("KeePass"), 6, 8),
                         session(QStringLiteral("CLion"), 8, 9)};
  service.setTrackingEnabled(true);
  service.setTrackingEnabled(false);

  QCOMPARE(repository.inserted.size(), 2);
  QCOMPARE(repository.inserted.at(0).appName, QStringLiteral("Telegram"));
  QVERIFY2(repository.inserted.at(0).title.isEmpty(),
           "the held-back session is written without its title");
  QCOMPARE(repository.inserted.at(1).appName, QStringLiteral("CLion"));
  QCOMPARE(repository.inserted.at(1).title, QStringLiteral("CLion"));
}

void TestActivityService::applyPrivacyToHistoryRedactsWhatIsStored() {
  FakeActivityRepository repository;
  repository.inserted = {session(QStringLiteral("KeePass"), 0, 1),
                         session(QStringLiteral("Telegram"), 1, 2),
                         session(QStringLiteral("Telegram"), 2, 3),
                         session(QStringLiteral("CLion"), 3, 4)};
  auto provider = std::make_unique<MockActivityProvider>();
  MockActivityProvider *mock = provider.get();
  ActivityService service(repository, std::move(provider));
  service.setPrivacyRules(kPrivacy);
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);

  // Still in the provider: written by the flush that comes first.
  mock->pendingEvents = {session(QStringLiteral("Telegram"), 4, 5)};

  QCOMPARE(service.applyPrivacyToHistory(), 3);

  QCOMPARE(repository.removed.size(), 1);
  QCOMPARE(repository.removed.first().appName, QStringLiteral("KeePass"));
  QCOMPARE(repository.inserted.size(), 4);
  for (const Activity &activity : std::as_const(repository.inserted)) {
    QVERIFY(activity.appName != QStringLiteral("KeePass"));
    if (activity.appName == QStringLiteral("Telegram")) {
      QVERIFY(activity.title.isEmpty());
    }
  }
  QCOMPARE(repository.inserted.at(2).title, QStringLiteral("CLion"));
  QVERIFY2(recorded.count() == 2,
           "the views refresh after the flush and again after the change");

  recorded.clear();
  QCOMPARE(service.applyPrivacyToHistory(), 0);
  QVERIFY2(recorded.isEmpty(), "nothing changed, nothing to refresh");
}

void TestActivityService::applyPrivacyToHistoryReportsFailure() {
  FakeActivityRepository repository;
  repository.inserted = {session(QStringLiteral("KeePass"), 0, 1)};
  repository.failRedact = true;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());
  service.setPrivacyRules(kPrivacy);
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);

  QCOMPARE(service.applyPrivacyToHistory(), -1);
  QVERIFY(recorded.isEmpty());

  // An unreadable history is not an empty one: "already follows the rules"
  // would be a lie.
  repository.failRedact = false;
  repository.failWindows = true;
  QCOMPARE(service.applyPrivacyToHistory(), -1);
  QVERIFY(repository.removed.isEmpty());
  QCOMPARE(repository.inserted.size(), 1);
}

// The whole write path with only the OS mocked, as above, for privacy.
void TestActivityService::privateWindowsNeverReachTheRepository() {
  FakeActivityRepository repository;
  ForegroundScript script;
  ActivityService service(repository, script.takeProvider());
  service.setPrivacyRules(kPrivacy);

  script.hold(QStringLiteral("CLion"), QStringLiteral("main.cpp"), 4);
  script.hold(QStringLiteral("KeePassXC"), QStringLiteral("bank.kdbx"), 5);
  script.hold(QStringLiteral("Telegram Desktop"), QStringLiteral("Olena"), 6);
  script.leave();

  service.setTrackingEnabled(false);

  QCOMPARE(repository.inserted.size(), 2);
  QCOMPARE(repository.inserted.at(0).title, QStringLiteral("main.cpp"));
  QCOMPARE(repository.inserted.at(1).appName,
           QStringLiteral("Telegram Desktop"));
  QCOMPARE(repository.inserted.at(1).title, QString());
  QCOMPARE(repository.inserted.at(1).durationSeconds(), 6);
}

QTEST_GUILESS_MAIN(TestActivityService)
#include "tst_ActivityService.moc"
