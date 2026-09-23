#include "domain/activity/CategoryRules.hpp"
#include "fakes/ForegroundScript.hpp"

#include <QTest>

#include <memory>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const QString kChrome = QStringLiteral("Google Chrome");
const QString kSlack = QStringLiteral("Slack");

// The same key ActivityService installs: the category of the window.
IUserActivityProvider::SessionKey categoryKey() {
  return [rules = CategoryRules({
              CategoryRule{QStringLiteral("Distractions"),
                           {QStringLiteral("Chrome")},
                           QStringLiteral("YouTube")},
              CategoryRule{
                  QStringLiteral("Browsing"), {QStringLiteral("Chrome")}, {}},
          })](const QString &app, const QString &title) {
    return rules.categorize(app, title);
  };
}

} // namespace

class TestPollingActivityProvider : public QObject {
  Q_OBJECT

private slots:
  void init();
  void cleanup();

  void titleChangeWithoutKeyStaysOneSession();
  void categoryChangeSplitsTheSession();
  void tickingTitleInOneCategoryStaysOneSession();
  void appSwitchAlwaysSplits();
  void fragmentBelowMinimumIsDropped();
  void idleClosesAndResumes();
  void noForegroundWindowClosesTheSession();
  void drainSplitsWithoutDoubleCounting();
  void currentSessionReportsWholeSpan();
  void keyChangeAppliesFromTheNextPoll();

private:
  void hold(const QString &app, const QString &title, int seconds) {
    _script->hold(app, title, seconds);
  }

  std::unique_ptr<ForegroundScript> _script;
  PollingActivityProvider *_provider = nullptr;
};

void TestPollingActivityProvider::init() {
  _script = std::make_unique<ForegroundScript>();
  _provider = &_script->provider();
}

void TestPollingActivityProvider::cleanup() { _script.reset(); }

void TestPollingActivityProvider::titleChangeWithoutKeyStaysOneSession() {
  hold(kChrome, QStringLiteral("Pull Request"), 5);
  hold(kChrome, QStringLiteral("YouTube"), 5);
  _script->leave();

  const QList<Activity> sessions = _provider->drainEvents();
  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().title, QStringLiteral("YouTube"));
  QCOMPARE(sessions.first().durationSeconds(), 10);
}

void TestPollingActivityProvider::categoryChangeSplitsTheSession() {
  _provider->setSessionKey(categoryKey());

  hold(kChrome, QStringLiteral("Pull Request #3655"), 5);
  hold(kChrome, QStringLiteral("Cats - YouTube"), 7);
  hold(kChrome, QStringLiteral("Jira"), 4);
  _script->leave();

  const QList<Activity> sessions = _provider->drainEvents();
  QCOMPARE(sessions.size(), 3);

  QCOMPARE(sessions.at(0).title, QStringLiteral("Pull Request #3655"));
  QCOMPARE(sessions.at(0).startedOn, utc(10, 0, 0));
  QCOMPARE(sessions.at(0).endedOn, utc(10, 0, 5));

  QCOMPARE(sessions.at(1).title, QStringLiteral("Cats - YouTube"));
  QCOMPARE(sessions.at(1).startedOn, utc(10, 0, 5));
  QCOMPARE(sessions.at(1).endedOn, utc(10, 0, 12));

  QCOMPARE(sessions.at(2).title, QStringLiteral("Jira"));
  QCOMPARE(sessions.at(2).durationSeconds(), 4);

  for (const Activity &session : sessions) {
    QCOMPARE(session.appName, kChrome);
  }
}

void TestPollingActivityProvider::tickingTitleInOneCategoryStaysOneSession() {
  _provider->setSessionKey(categoryKey());

  for (int second = 10; second > 0; --second) {
    hold(kChrome, QStringLiteral("Sale ends in %1 s").arg(second), 1);
  }
  _script->leave();

  const QList<Activity> sessions = _provider->drainEvents();
  QVERIFY2(sessions.size() == 1,
           "a title rewritten every poll must not fragment the session");
  QCOMPARE(sessions.first().durationSeconds(), 10);
  QCOMPARE(sessions.first().title, QStringLiteral("Sale ends in 1 s"));
}

void TestPollingActivityProvider::appSwitchAlwaysSplits() {
  _provider->setSessionKey(categoryKey());

  hold(kChrome, QStringLiteral("Docs"), 3);
  hold(kSlack, QStringLiteral("general"), 3);
  hold(kChrome, QStringLiteral("Docs"), 3);
  _script->leave();

  const QList<Activity> sessions = _provider->drainEvents();
  QCOMPARE(sessions.size(), 3);
  QCOMPARE(sessions.at(1).appName, kSlack);
}

void TestPollingActivityProvider::fragmentBelowMinimumIsDropped() {
  _provider->setSessionKey(categoryKey());

  hold(kChrome, QStringLiteral("Docs"), 5);
  hold(kChrome, QStringLiteral("YouTube"), 1);
  hold(kChrome, QStringLiteral("Docs"), 5);
  _script->leave();

  const QList<Activity> sessions = _provider->drainEvents();
  QCOMPARE(sessions.size(), 2);
  QCOMPARE(sessions.at(0).title, QStringLiteral("Docs"));
  QCOMPARE(sessions.at(1).title, QStringLiteral("Docs"));
}

void TestPollingActivityProvider::idleClosesAndResumes() {
  hold(kChrome, QStringLiteral("Docs"), 4);

  _script->idleFor(30);
  QVERIFY(_provider->isIdle());
  QVERIFY(!_provider->currentSession().has_value());

  hold(kChrome, QStringLiteral("Docs"), 4);
  QVERIFY(!_provider->isIdle());
  _script->leave();

  const QList<Activity> sessions = _provider->drainEvents();
  QCOMPARE(sessions.size(), 2);
  QCOMPARE(sessions.at(0).endedOn, utc(10, 0, 4));
  QVERIFY2(sessions.at(1).startedOn == utc(10, 0, 34),
           "idle time is not counted as part of either session");
}

void TestPollingActivityProvider::noForegroundWindowClosesTheSession() {
  hold(kChrome, QStringLiteral("Docs"), 3);
  _script->leave();
  _script->pollFor(4);
  QVERIFY(!_provider->currentSession().has_value());
  QCOMPARE(_provider->drainEvents().size(), 1);
}

void TestPollingActivityProvider::drainSplitsWithoutDoubleCounting() {
  hold(kChrome, QStringLiteral("Docs"), 6);

  const QList<Activity> first = _provider->drainEvents();
  QCOMPARE(first.size(), 1);
  QCOMPARE(first.first().endedOn, utc(10, 0, 5));

  hold(kChrome, QStringLiteral("Docs"), 4);
  _script->leave();

  const QList<Activity> second = _provider->drainEvents();
  QCOMPARE(second.size(), 1);
  QVERIFY2(second.first().startedOn == first.first().endedOn,
           "the drained part is not handed over twice");
  QCOMPARE(second.first().endedOn, utc(10, 0, 10));
}

void TestPollingActivityProvider::currentSessionReportsWholeSpan() {
  hold(kChrome, QStringLiteral("Docs"), 6);
  _provider->drainEvents();

  const auto current = _provider->currentSession();
  QVERIFY(current.has_value());
  QVERIFY2(current->startedOn == utc(10, 0, 0),
           "the live session keeps its real start across drains");
  QCOMPARE(current->title, QStringLiteral("Docs"));
}

void TestPollingActivityProvider::keyChangeAppliesFromTheNextPoll() {
  hold(kChrome, QStringLiteral("YouTube"), 3);
  _provider->setSessionKey(categoryKey());
  hold(kChrome, QStringLiteral("YouTube"), 3);
  _script->leave();

  // The open session was started under the empty key, so the first poll with
  // real rules sees a key change and splits once. Nothing is lost.
  const QList<Activity> sessions = _provider->drainEvents();
  QCOMPARE(sessions.size(), 2);
  QCOMPARE(sessions.at(0).durationSeconds() + sessions.at(1).durationSeconds(),
           6);
}

QTEST_GUILESS_MAIN(TestPollingActivityProvider)
#include "tst_PollingActivityProvider.moc"
