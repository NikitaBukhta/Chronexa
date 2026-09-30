#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/GoalDigestService.hpp"
#include "fakes/Fakes.hpp"

#include <QSignalSpy>
#include <QTest>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const QString kDistractions = QStringLiteral("Distractions");
const QString kBrowser = QStringLiteral("Google Chrome");
const QString kVideo = QStringLiteral("YouTube - cats");

// 2026-09-22 is a Tuesday. Local time, as the service reads the wall clock.
QDateTime tuesday(int hour, int minute = 0) {
  return QDateTime(QDate(2026, 9, 22), QTime(hour, minute));
}

TrackingSchedule workHours() {
  TrackingSchedule schedule;
  schedule.enabled = true;
  schedule.start = QTime(9, 0);
  schedule.end = QTime(18, 0);
  schedule.days = TrackingSchedule::kWorkdays;
  return schedule;
}

} // namespace

class TestGoalDigestService : public QObject {
  Q_OBJECT

private slots:
  void init();

  void nothingBeforeItsTime();
  void sentOnceAtItsTime();
  void reportsYesterday();
  void sentAgainTheNextDay();
  void missedWhileClosedIsSentOnLaunch();
  void missedWhileAsleepIsSentOnWaking();
  void catchingUpCanBeTurnedOff();
  void alreadySentTodayStaysQuiet();
  void noGoalsNoSummary();
  void offSendsNothing();
  void nextDueMovesOnOnceSent();
  void aDayOffIsSkipped();

private:
  std::unique_ptr<FakeActivityRepository> _repository;
  std::unique_ptr<ActivityQueryService> _queries;
  std::unique_ptr<ManualClock> _clock;
  std::unique_ptr<GoalDigestService> _service;
};

void TestGoalDigestService::init() {
  _repository = std::make_unique<FakeActivityRepository>();
  _repository->titles.append({kBrowser, kVideo, 80 * 60 * 1000, 4, {}});
  _queries = std::make_unique<ActivityQueryService>(*_repository);
  _queries->setCategoryRules(CategoryRules({CategoryRule{
      kDistractions, {QStringLiteral("Chrome")}, QStringLiteral("YouTube")}}));
  _clock = std::make_unique<ManualClock>(tuesday(8, 0));
  _service = std::make_unique<GoalDigestService>(
      *_queries, [this]() { return _clock->now(); });
  _service->setGoals({{kDistractions, GoalKind::Limit, 60,
                       TrackingSchedule::kEveryDay}});
  _service->setSchedule(workHours());
}

void TestGoalDigestService::nothingBeforeItsTime() {
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _clock->advance(59 * 60);
  _service->check();
  QVERIFY(due.isEmpty());
  QVERIFY(!_service->lastSentDay().isValid());
}

void TestGoalDigestService::sentOnceAtItsTime() {
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  QSignalSpy sent(_service.get(), &GoalDigestService::lastSentDayChanged);

  _clock->advance(3600 + 40);
  _service->check();
  QCOMPARE(due.size(), 1);
  QVERIFY2(!due[0][0].value<GoalDigest>().overdue,
           "40 s after its time is on time");
  QCOMPARE(sent.size(), 1);
  QCOMPARE(sent[0][0].toDate(), tuesday(0).date());

  _clock->advance(60);
  _service->check();
  _clock->advance(5 * 3600);
  _service->check();
  QCOMPARE(due.size(), 1);
}

void TestGoalDigestService::reportsYesterday() {
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _clock->advance(3600);
  _service->check();

  QCOMPARE(_repository->lastFrom, QDateTime(QDate(2026, 9, 21), QTime(0, 0)));
  QCOMPARE(_repository->lastTo, tuesday(0));
  const auto digest = due[0][0].value<GoalDigest>();
  QCOMPARE(digest.reportedDay, QDate(2026, 9, 21));
  QCOMPARE(digest.results.size(), 1);
  QCOMPARE(digest.results[0].seconds, qint64(80 * 60));
  QCOMPARE(digest.metCount(), 0);
  QCOMPARE(digest.today.size(), 1);
}

void TestGoalDigestService::sentAgainTheNextDay() {
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _clock->advance(3600);
  _service->check();
  _clock->advance(24 * 3600);
  _service->check();
  QCOMPARE(due.size(), 2);
  QCOMPARE(_service->lastSentDay(), QDate(2026, 9, 23));
}

void TestGoalDigestService::missedWhileClosedIsSentOnLaunch() {
  // The app starts at two in the afternoon; nothing went out today.
  _clock->advance(6 * 3600);
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);

  _service->check();

  QCOMPARE(due.size(), 1);
  QVERIFY(due[0][0].value<GoalDigest>().overdue);
  QCOMPARE(_service->lastSentDay(), tuesday(0).date());
}

void TestGoalDigestService::missedWhileAsleepIsSentOnWaking() {
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _clock->advance(59 * 60);
  _service->check();
  QVERIFY(due.isEmpty());

  // The lid closed at 08:59 and opened at 11:30.
  _clock->advance(2 * 3600 + 31 * 60);
  _service->check();
  QCOMPARE(due.size(), 1);
  QVERIFY(due[0][0].value<GoalDigest>().overdue);
}

void TestGoalDigestService::catchingUpCanBeTurnedOff() {
  _service->setCatchUp(false);
  _clock->advance(6 * 3600);
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  QSignalSpy sent(_service.get(), &GoalDigestService::lastSentDayChanged);

  _service->check();

  QVERIFY(due.isEmpty());
  QVERIFY2(sent.size() == 1, "the missed day is settled, not retried later");
  _service->setCatchUp(true);
  _service->check();
  QVERIFY(due.isEmpty());
}

void TestGoalDigestService::alreadySentTodayStaysQuiet() {
  _service->setLastSentDay(tuesday(0).date());
  _clock->advance(2 * 3600);
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _service->check();
  QVERIFY2(due.isEmpty(), "a restart after the summary does not repeat it");
}

void TestGoalDigestService::noGoalsNoSummary() {
  _service->setGoals({});
  _clock->advance(3600);
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _service->check();
  QVERIFY(due.isEmpty());
  QCOMPARE(_service->lastSentDay(), tuesday(0).date());
}

void TestGoalDigestService::offSendsNothing() {
  _service->setPlan({DigestTime::Off, QTime(9, 0)});
  _clock->advance(3 * 3600);
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _service->check();
  QVERIFY(due.isEmpty());
  QVERIFY(!_service->nextDue().isValid());
}

void TestGoalDigestService::nextDueMovesOnOnceSent() {
  QSignalSpy changed(_service.get(), &GoalDigestService::nextDueChanged);
  QCOMPARE(_service->nextDue(), tuesday(9));

  _service->setPlan({DigestTime::Custom, QTime(7, 30)});
  QCOMPARE(changed.size(), 1);
  QVERIFY2(_service->nextDue() == tuesday(7, 30).addDays(1),
           "today's 07:30 has passed: tomorrow's is next");

  _clock->advance(3600);
  _service->check();
  QCOMPARE(_service->nextDue(), tuesday(7, 30).addDays(1));
}

void TestGoalDigestService::aDayOffIsSkipped() {
  // Saturday at noon: no work start today, so nothing is overdue either.
  _clock->advance(4 * 24 * 3600 + 4 * 3600);
  QSignalSpy due(_service.get(), &GoalDigestService::digestDue);
  _service->check();
  QVERIFY(due.isEmpty());
  QCOMPARE(_service->nextDue(), tuesday(9).addDays(6));
}

QTEST_GUILESS_MAIN(TestGoalDigestService)
#include "tst_GoalDigestService.moc"
