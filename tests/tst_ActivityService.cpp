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

QTEST_GUILESS_MAIN(TestActivityService)
#include "tst_ActivityService.moc"
