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

  void editIsStoredAndAnnounced();
  void editWithoutApplicationIsRefused();
  void editIntoExcludedWindowIsRefused();
  void editIntoHiddenTitleIsStoredHidden();
  void failedEditIsReported();
  void cutIsClippedToTheSession();
  void cutOutsideTheSessionIsRefused();
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

void TestActivityService::editIsStoredAndAnnounced() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);

  const Activity shown = session(QStringLiteral("Google Chrome"), 0, 45);
  QCOMPARE(service.editSession(shown, SessionEdit{QStringLiteral(" Chrome "),
                                                  QStringLiteral(" Course "),
                                                  QStringLiteral(" Learning ")}),
           EditResult::Done);

  QCOMPARE(repository.edits.size(), 1);
  const auto &call = repository.edits.first();
  QCOMPARE(call.window.appName, QStringLiteral("Google Chrome"));
  QCOMPARE(call.from, utc(10, 0));
  QCOMPARE(call.to, utc(10, 45));
  QCOMPARE(call.edit.appName, QStringLiteral("Chrome"));
  QCOMPARE(call.edit.title, QStringLiteral("Course"));
  QCOMPARE(call.edit.category,
           std::optional<QString>(QStringLiteral("Learning")));
  QVERIFY2(recorded.count() == 1, "the views refresh after an edit");
}

void TestActivityService::editWithoutApplicationIsRefused() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());

  QCOMPARE(service.editSession(session(QStringLiteral("CLion"), 0, 5),
                               SessionEdit{QStringLiteral("  "),
                                           QStringLiteral("a"), std::nullopt}),
           EditResult::Invalid);
  Activity backwards = session(QStringLiteral("CLion"), 5, 0);
  QCOMPARE(service.editSession(backwards, SessionEdit{QStringLiteral("CLion"),
                                                      {}, std::nullopt}),
           EditResult::Invalid);
  QVERIFY(repository.edits.isEmpty());
}

void TestActivityService::editIntoExcludedWindowIsRefused() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());
  service.setPrivacyRules(kPrivacy);
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);

  QCOMPARE(service.editSession(session(QStringLiteral("CLion"), 0, 5),
                               SessionEdit{QStringLiteral("KeePassXC"),
                                           QStringLiteral("bank.kdbx"),
                                           std::nullopt}),
           EditResult::Excluded);
  QVERIFY(repository.edits.isEmpty());
  QVERIFY(recorded.isEmpty());
}

void TestActivityService::editIntoHiddenTitleIsStoredHidden() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());
  service.setPrivacyRules(kPrivacy);

  QCOMPARE(service.editSession(session(QStringLiteral("Chrome"), 0, 5),
                               SessionEdit{QStringLiteral("Telegram"),
                                           QStringLiteral("Olena"),
                                           std::nullopt}),
           EditResult::Done);
  QCOMPARE(repository.edits.size(), 1);
  QCOMPARE(repository.edits.first().edit.appName, QStringLiteral("Telegram"));
  QVERIFY2(repository.edits.first().edit.title.isEmpty() &&
               !repository.edits.first().edit.title.isNull(),
           "hidden as the tracker would have hidden it, and never NULL");
}

void TestActivityService::failedEditIsReported() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);
  const Activity shown = session(QStringLiteral("CLion"), 0, 5);
  const SessionEdit edit{QStringLiteral("CLion"), {}, std::nullopt};

  repository.editResult = -1;
  QCOMPARE(service.editSession(shown, edit), EditResult::Failed);
  QCOMPARE(service.cutSession(shown, utc(10, 1), utc(10, 2)),
           EditResult::Failed);

  // Gone in the meantime, e.g. cleared: nothing matched, nothing changed.
  repository.editResult = 0;
  QCOMPARE(service.editSession(shown, edit), EditResult::Failed);
  QVERIFY(recorded.isEmpty());
}

void TestActivityService::cutIsClippedToTheSession() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());
  QSignalSpy recorded(&service, &ActivityService::activityRecorded);

  const Activity shown = session(QStringLiteral("Chrome"), 10, 40);
  QCOMPARE(service.cutSession(shown, utc(10, 30), utc(11, 0)),
           EditResult::Done);

  QCOMPARE(repository.cuts.size(), 1);
  QCOMPARE(repository.cuts.first().window.appName, QStringLiteral("Chrome"));
  QCOMPARE(repository.cuts.first().from, utc(10, 30));
  QVERIFY2(repository.cuts.first().to == utc(10, 40),
           "never past the session, into whatever came after it");
  QCOMPARE(recorded.count(), 1);
}

void TestActivityService::cutOutsideTheSessionIsRefused() {
  FakeActivityRepository repository;
  ActivityService service(repository,
                          std::make_unique<MockActivityProvider>());

  const Activity shown = session(QStringLiteral("Chrome"), 10, 40);
  QCOMPARE(service.cutSession(shown, utc(10, 40), utc(10, 50)),
           EditResult::Invalid);
  QCOMPARE(service.cutSession(shown, utc(10, 30), utc(10, 20)),
           EditResult::Invalid);
  QCOMPARE(service.cutSession(shown, QDateTime(), utc(10, 20)),
           EditResult::Invalid);
  QVERIFY(repository.cuts.isEmpty());
}

QTEST_GUILESS_MAIN(TestActivityService)
#include "tst_ActivityService.moc"
