// The goal editor from the settings page and the goal card from the day page,
// driven through the real QML over the real models: AppSettings, GoalsModel,
// GoalService and GoalController. The history is a fake repository and the
// tray a fake notifier, so this runs headless under `bootstrap.py test`.
//
// The window is as narrow as the settings page gets at the minimum window
// size. Set CHRONEXA_UI_SHOTS to a directory to get a screenshot of each step.

#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/GoalService.hpp"
#include "core/AppSettings.hpp"
#include "fakes/Fakes.hpp"
#include "fakes/QmlHarness.hpp"
#include "ui/activity/CategoryRulesModel.hpp"
#include "ui/activity/GoalController.hpp"
#include "ui/activity/GoalsModel.hpp"
#include "ui/settings/SettingsController.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QSettings>
#include <QTest>
#include <QTimeZone>

#include <memory>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;
using chronexa::core::AppSettings;
using chronexa::settings::SettingsController;

namespace {

const QString kBrowser = QStringLiteral("Google Chrome");
const QString kVideo = QStringLiteral("YouTube - cats");

} // namespace

class TestGoalsUi : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void editorShowsTheSeededGoals();
  void addAGoalAndMakeItATarget();
  void setMinutesAndDays();
  void pickACategory();
  void removeAGoal();
  void dayCardFollowsProgress();

private:
  QQuickWindow *window() const;
  QQuickItem *root() const;
  // The editor row whose category picker shows `category`.
  QQuickItem *editorRow(const QString &category) const;
  void shot(const QString &name) const;
  void click(QQuickItem *item) const;

  std::unique_ptr<AppSettings> _settings;
  std::unique_ptr<CategoryRulesModel> _categoryRules;
  std::unique_ptr<GoalsModel> _goals;
  std::unique_ptr<SettingsController> _settingsController;
  std::unique_ptr<FakeActivityRepository> _repository;
  std::unique_ptr<ActivityQueryService> _queries;
  std::unique_ptr<ManualClock> _clock;
  std::unique_ptr<GoalService> _service;
  FakeNotifier _notifier;
  std::unique_ptr<GoalController> _controller;
  SourceModule _sourceModule;
  std::unique_ptr<QQmlApplicationEngine> _engine;
};

void TestGoalsUi::initTestCase() {
  QCoreApplication::setOrganizationName(QStringLiteral("ChronexaTests"));
  QCoreApplication::setApplicationName(QStringLiteral("tst_GoalsUi"));
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     QDir::tempPath());
  // Weekday labels and durations in English, whatever the machine's locale.
  QLocale::setDefault(QLocale(QLocale::English, QLocale::UnitedStates));
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
}

void TestGoalsUi::init() {
  QSettings().clear();
  _settings = std::make_unique<AppSettings>();
  _categoryRules = std::make_unique<CategoryRulesModel>(_settings.get());
  _goals = std::make_unique<GoalsModel>(_settings.get());
  _settingsController = std::make_unique<SettingsController>(
      _settings.get(), nullptr, nullptr);

  _repository = std::make_unique<FakeActivityRepository>();
  _queries = std::make_unique<ActivityQueryService>(*_repository);
  _queries->setCategoryRules(CategoryRules(_settings->categoryRules()));
  // A Monday, so the Mon-Fri target applies too.
  _clock = std::make_unique<ManualClock>(
      QDateTime(QDate(2026, 9, 21), QTime(15, 0), QTimeZone::UTC));
  _service = std::make_unique<GoalService>(
      *_queries, [this]() { return _clock->now(); });
  _service->setGoals(_settings->dailyGoals());
  connect(_settings.get(), &AppSettings::dailyGoalsChanged, _service.get(),
          [this]() { _service->setGoals(_settings->dailyGoals()); });
  _notifier = FakeNotifier();
  _controller = std::make_unique<GoalController>(
      _service.get(), _settings.get(), &_notifier);

  _engine = std::make_unique<QQmlApplicationEngine>();
  _engine->addUrlInterceptor(&_sourceModule);
  _engine->addImportPath(QStringLiteral(CHRONEXA_QML_IMPORT_DIR));
  QQmlContext *context = _engine->rootContext();
  context->setContextProperty("settingsController", _settingsController.get());
  context->setContextProperty("dailyGoals", _goals.get());
  context->setContextProperty("goalController", _controller.get());
  _engine->loadData(R"(
      import QtQuick
      import QtQuick.Layouts
      import Chronexa
      Window {
          width: 810
          height: 620
          visible: true
          color: Theme.plane
          ColumnLayout {
              anchors.fill: parent
              anchors.margins: Theme.gapLoose
              spacing: Theme.gap
              Card {
                  Layout.fillWidth: true
                  title: "Daily goals"
                  GoalList { objectName: "goalList"; Layout.fillWidth: true; goalsModel: dailyGoals }
              }
              Card {
                  Layout.fillWidth: true
                  visible: goalController.hasGoals
                  title: "Today's goals"
                  subtitle: goalController.summary
                  GoalProgressList { objectName: "goalProgress"; Layout.fillWidth: true; progressModel: goalController.progress }
              }
              Item { Layout.fillHeight: true }
          }
      })");
  QVERIFY2(window() != nullptr, "the goal views load");
  QVERIFY(QTest::qWaitForWindowExposed(window()));
  QTRY_VERIFY(editorRow(QStringLiteral("Distractions")) != nullptr);
}

void TestGoalsUi::cleanup() {
  _engine.reset();
  _controller.reset();
  _service.reset();
  _queries.reset();
  _repository.reset();
  _settingsController.reset();
  _goals.reset();
  _categoryRules.reset();
  _settings.reset();
  QSettings().clear();
}

QQuickWindow *TestGoalsUi::window() const {
  const QList<QObject *> roots = _engine->rootObjects();
  return roots.isEmpty() ? nullptr
                         : qobject_cast<QQuickWindow *>(roots.first());
}

QQuickItem *TestGoalsUi::root() const { return window()->contentItem(); }

QQuickItem *TestGoalsUi::editorRow(const QString &category) const {
  QQuickItem *list = byName(root(), QStringLiteral("goalList"));
  QQuickItem *picker = button(list, category);
  return picker != nullptr ? picker->parentItem() : nullptr;
}

void TestGoalsUi::shot(const QString &name) const {
  const QString dir = qEnvironmentVariable("CHRONEXA_UI_SHOTS");
  if (!dir.isEmpty()) {
    QTest::qWait(50);
    window()->grabWindow().save(QDir(dir).filePath(name + ".png"));
  }
}

void TestGoalsUi::click(QQuickItem *item) const {
  QVERIFY(item != nullptr);
  const QPointF centre =
      item->mapToScene(QPointF(item->width() / 2, item->height() / 2));
  QTest::mouseClick(window(), Qt::LeftButton, {}, centre.toPoint());
}

void TestGoalsUi::editorShowsTheSeededGoals() {
  shot(QStringLiteral("goals-0-seeded"));

  QQuickItem *distractions = editorRow(QStringLiteral("Distractions"));
  QQuickItem *work = editorRow(QStringLiteral("Work"));
  QVERIFY(distractions != nullptr && work != nullptr);
  QVERIFY(button(distractions, QStringLiteral("At most"))
              ->property("active")
              .toBool());
  QVERIFY(button(work, QStringLiteral("At least"))
              ->property("active")
              .toBool());
  QCOMPARE(textField(work)->property("text").toString(),
           QStringLiteral("04:00"));

  // Every control of a row stays inside the card at the narrowest width.
  QQuickItem *list = byName(root(), QStringLiteral("goalList"));
  const qreal right = list->mapToScene(QPointF(list->width(), 0)).x();
  const QList<QQuickItem *> controls = work->childItems();
  for (QQuickItem *control : controls) {
    const qreal edge = control->mapToScene(QPointF(control->width(), 0)).x();
    QVERIFY2(edge <= right + 0.5,
             qPrintable(QStringLiteral("%1 ends at %2, the list at %3")
                            .arg(control->metaObject()->className())
                            .arg(edge)
                            .arg(right)));
  }
}

void TestGoalsUi::addAGoalAndMakeItATarget() {
  click(button(root(), QStringLiteral("Add goal")));
  QTRY_VERIFY(editorRow(QStringLiteral("Communication")) != nullptr);
  QCOMPARE(_settings->dailyGoals().size(), 3);

  click(button(editorRow(QStringLiteral("Communication")),
               QStringLiteral("At least")));
  QTRY_COMPARE(_settings->dailyGoals()[2].kind, GoalKind::Target);
  shot(QStringLiteral("goals-1-added"));
}

void TestGoalsUi::setMinutesAndDays() {
  QQuickItem *row = editorRow(QStringLiteral("Distractions"));
  QQuickItem *minutes = textField(row);
  click(minutes);
  QTRY_VERIFY(minutes->hasActiveFocus());
  QTest::keyClick(window(), Qt::Key_Home);
  for (const char c : QByteArray("0045")) {
    QTest::keyClick(window(), c);
  }
  QTest::keyClick(window(), Qt::Key_Return);
  QTRY_COMPARE(_settings->dailyGoals()[0].minutes, 45);

  click(label(row, QStringLiteral("Sat")));
  click(label(row, QStringLiteral("Sun")));
  QTRY_COMPARE(_settings->dailyGoals()[0].days, TrackingSchedule::kWorkdays);
  shot(QStringLiteral("goals-2-edited"));
}

void TestGoalsUi::pickACategory() {
  click(button(editorRow(QStringLiteral("Distractions")),
               QStringLiteral("Distractions")));

  // The choices open in the overlay, outside the row.
  QQuickItem *overlay = findItem(root(), [](QQuickItem *item) {
    return item->inherits("QQuickOverlay");
  });
  QVERIFY(overlay != nullptr);
  QTRY_VERIFY(button(overlay, QStringLiteral("Communication")) != nullptr);
  shot(QStringLiteral("goals-3-picker"));
  click(button(overlay, QStringLiteral("Communication")));

  QTRY_COMPARE(_settings->dailyGoals()[0].category,
               QStringLiteral("Communication"));
  QVERIFY(editorRow(QStringLiteral("Distractions")) == nullptr);
}

void TestGoalsUi::removeAGoal() {
  QQuickItem *remove =
      findItem(editorRow(QStringLiteral("Work")), [](QQuickItem *item) {
        return item->inherits("QQuickAbstractButton") &&
               item->property("glyph").toString() == QStringLiteral(u"✕");
      });
  click(remove);

  QTRY_COMPARE(_settings->dailyGoals().size(), 1);
  QVERIFY(editorRow(QStringLiteral("Work")) == nullptr);
}

void TestGoalsUi::dayCardFollowsProgress() {
  QQuickItem *card = byName(root(), QStringLiteral("goalProgress"));
  QVERIFY(card != nullptr);
  QVERIFY(label(card, QStringLiteral("0m of 1h")) != nullptr);
  QVERIFY(label(card, QStringLiteral("4h to go")) != nullptr);
  QVERIFY(label(root(), QStringLiteral("1 of 2 on track")) != nullptr);

  _repository->titles.append({kBrowser, kVideo, 70 * 60 * 1000, 3, {}});
  _repository->titles.append({QStringLiteral("CLion"),
                              QStringLiteral("main.cpp"), 250 * 60 * 1000, 9,
                              {}});
  _service->refresh();

  QTRY_VERIFY(label(card, QStringLiteral("10m over")) != nullptr);
  QVERIFY(label(card, QStringLiteral("1h 10m of 1h")) != nullptr);
  QVERIFY(label(card, QStringLiteral("Reached")) != nullptr);
  QVERIFY(label(root(), QStringLiteral("1 of 2 on track")) != nullptr);
  QCOMPARE(_notifier.messages.size(), 2);
  shot(QStringLiteral("goals-4-progress"));
}

QTEST_MAIN(TestGoalsUi)
#include "tst_GoalsUi.moc"
