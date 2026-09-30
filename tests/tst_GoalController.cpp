#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/GoalDigestService.hpp"
#include "application/activity/GoalService.hpp"
#include "core/AppSettings.hpp"
#include "fakes/Fakes.hpp"
#include "ui/activity/ActivityFormat.hpp"
#include "ui/activity/GoalController.hpp"

#include <QCoreApplication>
#include <QSettings>
#include <QSignalSpy>
#include <QTest>
#include <QTimeZone>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;
using chronexa::core::AppSettings;

namespace {

const QString kDistractions = QStringLiteral("Distractions");
const QString kWork = QStringLiteral("Work");
const QString kBrowser = QStringLiteral("Google Chrome");
const QString kVideo = QStringLiteral("YouTube - cats");

DailyGoal limit(const QString &category, int minutes) {
  return {category, GoalKind::Limit, minutes, TrackingSchedule::kEveryDay};
}

DailyGoal target(const QString &category, int minutes) {
  return {category, GoalKind::Target, minutes, TrackingSchedule::kEveryDay};
}

GoalProgress progress(const DailyGoal &goal, int minutes) {
  return {goal, qint64(minutes) * 60};
}

} // namespace

Q_DECLARE_METATYPE(chronexa::activity::GoalProgress)

class TestGoalController : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void formatTexts_data();
  void formatTexts();
  void alertTexts();

  void progressIsExposed();
  void crossingNotifies();
  void notificationsCanBeTurnedOff();
  void unsupportedTrayIsReported();
  void colourFollowsCategoryOrder();

  void digestTexts();
  void digestIsNotified();

private:
  void setUpService();
  void spend(const QString &app, const QString &title, int minutes,
             std::optional<QString> category = std::nullopt);

  std::unique_ptr<AppSettings> _settings;
  std::unique_ptr<FakeActivityRepository> _repository;
  std::unique_ptr<ActivityQueryService> _queries;
  std::unique_ptr<ManualClock> _clock;
  std::unique_ptr<GoalService> _service;
  FakeNotifier _notifier;
  std::unique_ptr<GoalController> _controller;
};

void TestGoalController::initTestCase() {
  QCoreApplication::setOrganizationName(QStringLiteral("ChronexaTests"));
  QCoreApplication::setApplicationName(QStringLiteral("tst_GoalController"));
}

void TestGoalController::init() {
  QSettings().clear();
  _settings = std::make_unique<AppSettings>();
  _settings->setCategoryRules({
      CategoryRule{kWork, {QStringLiteral("CLion")}, {}},
      CategoryRule{kDistractions, {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
  });
  _repository = std::make_unique<FakeActivityRepository>();
  _queries = std::make_unique<ActivityQueryService>(*_repository);
  _queries->setCategoryRules(CategoryRules(_settings->categoryRules()));
  _clock = std::make_unique<ManualClock>(
      QDateTime(QDate(2026, 9, 21), QTime(10, 0), QTimeZone::UTC));
  _notifier = FakeNotifier();
  setUpService();
}

void TestGoalController::cleanup() {
  _controller.reset();
  _service.reset();
  _queries.reset();
  _repository.reset();
  _settings.reset();
  QSettings().clear();
}

void TestGoalController::setUpService() {
  _service = std::make_unique<GoalService>(
      *_queries, [this]() { return _clock->now(); });
  _service->setGoals({limit(kDistractions, 60), target(kWork, 240)});
  _controller = std::make_unique<GoalController>(
      _service.get(), _settings.get(), &_notifier);
}

void TestGoalController::spend(const QString &app, const QString &title,
                               int minutes, std::optional<QString> category) {
  _repository->titles.append(
      {app, title, qint64(minutes) * 60 * 1000, 1, std::move(category)});
}

void TestGoalController::formatTexts_data() {
  QTest::addColumn<GoalProgress>("item");
  QTest::addColumn<QString>("amount");
  QTest::addColumn<QString>("status");

  QTest::newRow("limit with room")
      << progress(limit(kDistractions, 60), 45) << QStringLiteral("45m of 1h")
      << QStringLiteral("15m left");
  QTest::newRow("limit gone past")
      << progress(limit(kDistractions, 60), 72)
      << QStringLiteral("1h 12m of 1h") << QStringLiteral("12m over");
  QTest::newRow("target short")
      << progress(target(kWork, 240), 50) << QStringLiteral("50m of 4h")
      << QStringLiteral("3h 10m to go");
  QTest::newRow("target reached")
      << progress(target(kWork, 240), 250) << QStringLiteral("4h 10m of 4h")
      << QStringLiteral("Reached");
  QTest::newRow("nothing yet")
      << progress(target(kWork, 90), 0) << QStringLiteral("0m of 1h 30m")
      << QStringLiteral("1h 30m to go");
}

void TestGoalController::formatTexts() {
  QFETCH(GoalProgress, item);
  QFETCH(QString, amount);
  QFETCH(QString, status);

  QCOMPARE(format::goalAmount(item), amount);
  QCOMPARE(format::goalStatus(item), status);
}

void TestGoalController::alertTexts() {
  const GoalProgress over = progress(limit(kDistractions, 60), 61);
  QCOMPARE(format::goalAlertTitle(over),
           QStringLiteral("Limit passed: Distractions"));
  QCOMPARE(format::goalAlertMessage(over),
           QStringLiteral("1h 1m today, the limit is 1h."));

  const GoalProgress done = progress(target(kWork, 240), 240);
  QCOMPARE(format::goalAlertTitle(done), QStringLiteral("Goal reached: Work"));
  QCOMPARE(format::goalAlertMessage(done),
           QStringLiteral("4h today, the goal was 4h. Well done."));

  QCOMPARE(format::goalKind(GoalKind::Limit), QStringLiteral("at most"));
  QCOMPARE(format::goalKind(GoalKind::Target), QStringLiteral("at least"));
}

void TestGoalController::progressIsExposed() {
  spend(kBrowser, kVideo, 30);
  _service->refresh();

  QVERIFY(_controller->hasGoals());
  QCOMPARE(_controller->onTrackCount(), 1);
  QCOMPARE(_controller->summary(), QStringLiteral("1 of 2 on track"));

  GoalProgressModel *model = _controller->progress();
  QCOMPARE(model->rowCount(), 2);
  const QModelIndex first = model->index(0);
  QCOMPARE(model->data(first, GoalProgressModel::CategoryRole).toString(),
           kDistractions);
  QCOMPARE(model->data(first, GoalProgressModel::StateRole).toString(),
           QStringLiteral("within"));
  QCOMPARE(model->data(first, GoalProgressModel::FractionRole).toDouble(),
           0.5);
  QCOMPARE(model->data(first, GoalProgressModel::StatusTextRole).toString(),
           QStringLiteral("30m left"));
  QCOMPARE(model->data(model->index(1), GoalProgressModel::StateRole)
               .toString(),
           QStringLiteral("short"));

  QSignalSpy changed(model, &QAbstractItemModel::dataChanged);
  QSignalSpy reset(model, &QAbstractItemModel::modelReset);
  spend(kBrowser, kVideo, 40);
  _service->refresh();
  QVERIFY2(changed.size() == 1 && reset.isEmpty(),
           "a minute's progress updates rows in place");
  QCOMPARE(model->data(first, GoalProgressModel::StateRole).toString(),
           QStringLiteral("exceeded"));
  QCOMPARE(_controller->summary(), QStringLiteral("0 of 2 on track"));

  _service->setGoals({});
  QVERIFY(!_controller->hasGoals());
  QCOMPARE(model->rowCount(), 0);
}

void TestGoalController::crossingNotifies() {
  spend(kBrowser, kVideo, 50);
  _service->refresh();
  QVERIFY(_notifier.messages.isEmpty());

  spend(kBrowser, kVideo, 20);
  spend(QStringLiteral("CLion"), QStringLiteral("main.cpp"), 250);
  _service->refresh();

  QCOMPARE(_notifier.messages.size(), 2);
  QCOMPARE(_notifier.messages[0].title,
           QStringLiteral("Limit passed: Distractions"));
  QCOMPARE(_notifier.messages[0].text,
           QStringLiteral("1h 10m today, the limit is 1h."));
  QCOMPARE(_notifier.messages[1].title, QStringLiteral("Goal reached: Work"));

  _service->refresh();
  QCOMPARE(_notifier.messages.size(), 2);
}

void TestGoalController::notificationsCanBeTurnedOff() {
  _settings->setGoalNotifications(false);
  spend(kBrowser, kVideo, 61);
  _service->refresh();
  QVERIFY(_notifier.messages.isEmpty());

  AppSettings reloaded;
  QVERIFY(!reloaded.goalNotifications());
}

void TestGoalController::unsupportedTrayIsReported() {
  QVERIFY(_controller->notificationsSupported());
  _notifier.supported = false;
  QVERIFY(!_controller->notificationsSupported());

  GoalController headless(_service.get(), _settings.get(), nullptr);
  QVERIFY(!headless.notificationsSupported());
  // No notifier at all: a crossing is dropped, not dereferenced.
  spend(kBrowser, kVideo, 61);
  _service->refresh();
}

void TestGoalController::colourFollowsCategoryOrder() {
  GoalProgressModel *model = _controller->progress();
  QCOMPARE(model->data(model->index(0), GoalProgressModel::ColorSlotRole)
               .toInt(),
           1);

  QList<CategoryRule> reordered = _settings->categoryRules();
  reordered.move(1, 0);
  _settings->setCategoryRules(reordered);

  QCOMPARE(model->data(model->index(0), GoalProgressModel::ColorSlotRole)
               .toInt(),
           0);
}

void TestGoalController::digestTexts() {
  GoalDigest digest;
  digest.reportedDay = QDate(2026, 9, 20);
  digest.results = {progress(target(kWork, 240), 250),
                    progress(limit(kDistractions, 60), 80)};
  digest.today = {limit(kDistractions, 60)};

  QCOMPARE(format::digestTitle(digest),
           QStringLiteral("Yesterday's goals: 1 of 2 met"));
  QCOMPARE(format::digestMessage(digest),
           QStringLiteral(u"✓ Work: 4h 10m of 4h\n"
                          u"✗ Distractions: 1h 20m of 1h\n"
                          u"Today: Distractions: at most 1h"));

  // A Monday after a free weekend: nothing to report, only today's plan.
  digest.results.clear();
  digest.today = {limit(kDistractions, 60), target(kWork, 240)};
  QCOMPARE(format::digestTitle(digest), QStringLiteral("Today's goals"));
  QCOMPARE(format::digestMessage(digest),
           QStringLiteral("Today: Distractions: at most 1h; "
                          "Work: at least 4h"));

  QCOMPARE(format::nextDigest({}, DigestTime::Off),
           QStringLiteral("No daily summary."));
  QVERIFY(format::nextDigest(QDateTime::currentDateTime().addSecs(60),
                             DigestTime::WorkStart)
              .startsWith(QStringLiteral("Next: ")));
}

void TestGoalController::digestIsNotified() {
  // Local time, as the summary's due moments are.
  ManualClock clock(QDateTime(QDate(2026, 9, 22), QTime(10, 0)));
  GoalDigestService digest(*_queries, [&clock]() { return clock.now(); });
  digest.setGoals({limit(kDistractions, 60)});
  digest.setPlan({DigestTime::Custom, QTime(9, 0)});
  GoalController controller(_service.get(), _settings.get(), &_notifier,
                            &digest);
  QSignalSpy changed(&controller, &GoalController::digestChanged);

  digest.check();
  QCOMPARE(_notifier.messages.size(), 1);
  QCOMPARE(_notifier.messages[0].title,
           QStringLiteral("Yesterday's goals: 1 of 1 met"));
  QVERIFY2(changed.size() >= 1, "the next summary moved on to tomorrow");

  clock.advance(24 * 3600);
  _settings->setGoalNotifications(false);
  digest.check();
  QVERIFY2(_notifier.messages.size() == 1,
           "the tray switch silences the summary too");
}

QTEST_GUILESS_MAIN(TestGoalController)
#include "tst_GoalController.moc"
