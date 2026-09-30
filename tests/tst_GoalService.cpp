#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/GoalService.hpp"
#include "fakes/Fakes.hpp"

#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const QString kBrowser = QStringLiteral("Google Chrome");
const QString kVideo = QStringLiteral("YouTube - cats");
const QString kIde = QStringLiteral("CLion");
const QString kDistractions = QStringLiteral("Distractions");
const QString kWork = QStringLiteral("Work");

// A Monday, in UTC so the day's edges do not depend on the machine.
QDateTime monday(int hour, int minute = 0) {
  return QDateTime(QDate(2026, 9, 21), QTime(hour, minute), QTimeZone::UTC);
}

DailyGoal limit(const QString &category, int minutes) {
  return {category, GoalKind::Limit, minutes, TrackingSchedule::kEveryDay};
}

DailyGoal target(const QString &category, int minutes) {
  return {category, GoalKind::Target, minutes, TrackingSchedule::kEveryDay};
}

} // namespace

class TestGoalService : public QObject {
  Q_OBJECT

private slots:
  void init();

  void measuresTodayFromMidnightToMidnight();
  void alreadyCrossedAtStartIsNotAnnounced();
  void crossingIsAnnouncedOnce();
  void targetReachedIsAnnounced();
  void aNewDayStartsOver();
  void aChangedGoalIsMeasuredAgain();
  void offDayGoalsAreLeftOut();
  void progressChangedOnlyOnChange();
  void logOverrideCountsForItsCategory();

private:
  void spend(const QString &app, const QString &title, int minutes);

  std::unique_ptr<FakeActivityRepository> _repository;
  std::unique_ptr<ActivityQueryService> _queries;
  std::unique_ptr<ManualClock> _clock;
  std::unique_ptr<GoalService> _service;
};

void TestGoalService::init() {
  _repository = std::make_unique<FakeActivityRepository>();
  _queries = std::make_unique<ActivityQueryService>(*_repository);
  _queries->setCategoryRules(CategoryRules({
      CategoryRule{kDistractions, {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
      CategoryRule{kWork, {kIde}, {}},
  }));
  _clock = std::make_unique<ManualClock>(monday(9));
  _service = std::make_unique<GoalService>(
      *_queries, [this]() { return _clock->now(); });
}

// The fake repository serves the same title totals for any range; a test
// adds to them as if the tracker had recorded more of the day.
void TestGoalService::spend(const QString &app, const QString &title,
                            int minutes) {
  for (TitleTotal &total : _repository->titles) {
    if (total.appName == app && total.title == title) {
      total.milliseconds += qint64(minutes) * 60 * 1000;
      ++total.sessionCount;
      return;
    }
  }
  _repository->titles.append({app, title, qint64(minutes) * 60 * 1000, 1, {}});
}

void TestGoalService::measuresTodayFromMidnightToMidnight() {
  _clock->advance(5 * 3600 + 17 * 60);
  _service->setGoals({limit(kDistractions, 60)});

  QCOMPARE(_repository->lastFrom, monday(0));
  QCOMPARE(_repository->lastTo, monday(0).addDays(1));
  QCOMPARE(_service->day(), QDate(2026, 9, 21));
}

void TestGoalService::alreadyCrossedAtStartIsNotAnnounced() {
  spend(kBrowser, kVideo, 90);
  QSignalSpy crossed(_service.get(), &GoalService::goalCrossed);

  _service->setGoals({limit(kDistractions, 60)});
  _service->refresh();

  QCOMPARE(_service->progress().size(), 1);
  QCOMPARE(_service->progress()[0].state(), GoalState::Exceeded);
  QVERIFY2(crossed.isEmpty(),
           "a restart must not repeat what the user was already told");
}

void TestGoalService::crossingIsAnnouncedOnce() {
  _service->setGoals({limit(kDistractions, 60)});
  QSignalSpy crossed(_service.get(), &GoalService::goalCrossed);

  spend(kBrowser, kVideo, 60);
  _service->refresh();
  QVERIFY2(crossed.isEmpty(), "exactly at the limit is still within it");

  spend(kBrowser, kVideo, 1);
  _service->refresh();
  QCOMPARE(crossed.size(), 1);
  const auto progress = crossed[0][0].value<GoalProgress>();
  QCOMPARE(progress.goal, limit(kDistractions, 60));
  QCOMPARE(progress.seconds, qint64(61 * 60));

  spend(kBrowser, kVideo, 10);
  _service->refresh();
  _service->refresh();
  QCOMPARE(crossed.size(), 1);
}

void TestGoalService::targetReachedIsAnnounced() {
  _service->setGoals({target(kWork, 240), limit(kDistractions, 60)});
  QSignalSpy crossed(_service.get(), &GoalService::goalCrossed);

  spend(kIde, QStringLiteral("main.cpp"), 239);
  _service->refresh();
  QVERIFY(crossed.isEmpty());

  spend(kIde, QStringLiteral("main.cpp"), 1);
  _service->refresh();
  QCOMPARE(crossed.size(), 1);
  QCOMPARE(crossed[0][0].value<GoalProgress>().goal.kind, GoalKind::Target);
}

void TestGoalService::aNewDayStartsOver() {
  _service->setGoals({limit(kDistractions, 60)});
  QSignalSpy crossed(_service.get(), &GoalService::goalCrossed);
  spend(kBrowser, kVideo, 61);
  _service->refresh();
  QCOMPARE(crossed.size(), 1);

  // Tuesday: the day is measured afresh...
  _clock->advance(24 * 3600);
  _repository->titles.clear();
  _service->refresh();
  QCOMPARE(_service->day(), QDate(2026, 9, 22));
  QCOMPARE(_repository->lastFrom, monday(0).addDays(1));
  QCOMPARE(_service->progress()[0].seconds, qint64(0));

  // ...and the same limit passed again is news again.
  spend(kBrowser, kVideo, 61);
  _service->refresh();
  QCOMPARE(crossed.size(), 2);
}

void TestGoalService::aChangedGoalIsMeasuredAgain() {
  _service->setGoals({limit(kDistractions, 60)});
  QSignalSpy crossed(_service.get(), &GoalService::goalCrossed);
  spend(kBrowser, kVideo, 61);
  _service->refresh();
  QCOMPARE(crossed.size(), 1);

  // Raised past today's time: back within it, nothing to say.
  _service->setGoals({limit(kDistractions, 90)});
  QCOMPARE(crossed.size(), 1);
  QCOMPARE(_service->progress()[0].state(), GoalState::Within);

  spend(kBrowser, kVideo, 30);
  _service->refresh();
  QVERIFY2(crossed.size() == 2, "the raised limit is its own goal");
}

void TestGoalService::offDayGoalsAreLeftOut() {
  DailyGoal weekend = target(kWork, 60);
  weekend.days = 0b1100000;
  _service->setGoals({weekend, limit(kDistractions, 60)});

  QCOMPARE(_service->progress().size(), 1);
  QCOMPARE(_service->progress()[0].goal.category, kDistractions);
}

void TestGoalService::progressChangedOnlyOnChange() {
  QSignalSpy changed(_service.get(), &GoalService::progressChanged);
  _service->setGoals({limit(kDistractions, 60)});
  QCOMPARE(changed.size(), 1);

  _service->refresh();
  QVERIFY2(changed.size() == 1, "a minute with nothing new stays quiet");

  spend(kBrowser, kVideo, 5);
  _service->refresh();
  QCOMPARE(changed.size(), 2);

  _service->setGoals({limit(kDistractions, 60)});
  QVERIFY2(changed.size() == 2, "setting the same goals is not a change");
}

void TestGoalService::logOverrideCountsForItsCategory() {
  // A video the user filed under Work from the log counts for Work.
  _repository->titles.append(
      {kBrowser, kVideo, qint64(120) * 60 * 1000, 1, kWork});
  _service->setGoals({target(kWork, 120), limit(kDistractions, 60)});

  QCOMPARE(_service->progress()[0].state(), GoalState::Reached);
  QCOMPARE(_service->progress()[1].seconds, qint64(0));
}

QTEST_GUILESS_MAIN(TestGoalService)
#include "tst_GoalService.moc"
