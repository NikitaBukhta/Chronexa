#include "AppInitializer.hpp"
#include "AppEnvironment.hpp"
#include "AppSettings.hpp"
#include "TranslationManager.hpp"

#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/ActivityService.hpp"
#include "application/activity/GoalDigestService.hpp"
#include "application/activity/GoalService.hpp"
#include "infrastructure/activity/SqliteActivityRepository.hpp"
#include "infrastructure/system/TrayNotifier.hpp"
#include "ui/activity/ActivityQueryController.hpp"
#include "ui/activity/CategoryRulesModel.hpp"
#include "ui/activity/GoalController.hpp"
#include "ui/activity/GoalsModel.hpp"
#include "ui/activity/PrivacyRulesModel.hpp"
#include "ui/activity/UserActivityController.hpp"
#include "ui/settings/SettingsController.hpp"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QLoggingCategory>
#include <QQmlContext>
#include <QQuickStyle>
#include <QWindow>
#include <QtSystemDetection>

#ifdef Q_OS_WIN
#include "infrastructure/system/WindowsAutoStartService.hpp"
using OSSpecificAutoStart = chronexa::system::WindowsAutoStartService;
#else
#error "Unsupported platform"
#endif

namespace {

Q_LOGGING_CATEGORY(lcInit, "chronexa.core.init")

constexpr int kDigestFirstCheckMs = 3000;

} // namespace

namespace chronexa::core {

AppInitializer::AppInitializer(QGuiApplication &app, QObject *parent)
    : QObject(parent), _app(app),
      _engine(std::make_unique<QQmlApplicationEngine>()) {
  // Before anything reads a setting or a path: both are keyed on these.
  QGuiApplication::setOrganizationName(QStringLiteral("Chronexa"));
  QGuiApplication::setOrganizationDomain(QStringLiteral("chronexa.local"));
  QGuiApplication::setApplicationName(QStringLiteral("Chronexa"));
}

AppInitializer::~AppInitializer() = default;

int AppInitializer::run() {
  init();
  const int res = QGuiApplication::exec();
  shutdown();
  return res;
}

void AppInitializer::shutdown() {
  // Tear down here rather than in the destructor so it happens while the file
  // logger is still installed: ~ActivityService does the final flush and is
  // the one place that reports sessions lost on the way out.
  _engine.reset();

  _monthQuery.reset();
  _logQuery.reset();
  _periodQuery.reset();
  _dayQuery.reset();
  _goalController.reset();
  _goalDigest.reset();
  _goalService.reset();
  _dailyGoals.reset();
  _privacyRules.reset();
  _categoryRules.reset();
  _activityController.reset();
  _activityService.reset();
  _activityQueryService.reset();
  _activityRepository.reset();

  _settingsController.reset();
  _tray.reset();
  _autoStart.reset();
  _translations.reset();
  _settings.reset();

  qCInfo(lcInit) << "Shut down cleanly";
  AppEnvironment::shutdownFileLogger();
}

void AppInitializer::applyCommandLine() {
  QCommandLineParser parser;
  const QCommandLineOption profileOption(
      QStringLiteral("profile"),
      QStringLiteral("Keep history, settings and logs in <dir>."),
      QStringLiteral("dir"));
  parser.addOption(profileOption);
  // parse(), not process(): an unknown argument must not stop the app.
  parser.parse(QCoreApplication::arguments());

  if (parser.isSet(profileOption)) {
    AppEnvironment::useProfileDir(parser.value(profileOption));
  }
}

void AppInitializer::init() {
  // The profile decides where the log goes, so it comes before the logger.
  applyCommandLine();
  AppEnvironment::installFileLogger();

#ifdef QT_NO_DEBUG
  QLoggingCategory::setFilterRules("chronexa.*.debug=false\n"
                                   "chronexa.*.info=false");
#else
  QLoggingCategory::setFilterRules("chronexa.*.debug=true");
#endif

  QQuickStyle::setStyle(QStringLiteral("Basic"));

  buildSettingsModule();
  buildActivityModule();
  registerQmlTypes();
}

void AppInitializer::buildSettingsModule() {
  _settings = std::make_unique<AppSettings>();
  _translations = std::make_unique<TranslationManager>(_engine.get());
  _autoStart = std::make_unique<OSSpecificAutoStart>();
  _tray = std::make_unique<system::TrayNotifier>();
  QObject::connect(_translations.get(), &TranslationManager::languageApplied,
                   _tray.get(), &system::TrayNotifier::retranslate);
  _settingsController = std::make_unique<settings::SettingsController>(
      _settings.get(), _autoStart.get(), _translations.get());

  _settingsController->applyStoredLanguage();

  qCInfo(lcInit) << "Settings module wired";
}

void AppInitializer::buildActivityModule() {
  _activityRepository = std::make_unique<activity::SqliteActivityRepository>(
      AppEnvironment::databasePath());
  if (!_activityRepository->open()) {
    qCWarning(lcInit) << "History unavailable -- the views will stay empty";
  }

  _activityQueryService =
      std::make_unique<activity::ActivityQueryService>(*_activityRepository);
  _activityService =
      std::make_unique<activity::ActivityService>(*_activityRepository);
  _privacyRules =
      std::make_unique<activity::PrivacyRulesModel>(_settings.get());
  applyPrivacyRules();
  QObject::connect(_settings.get(), &AppSettings::privacyRulesChanged,
                   _activityService.get(), [this]() { applyPrivacyRules(); });

  _categoryRules =
      std::make_unique<activity::CategoryRulesModel>(_settings.get());
  applyCategoryRules();
  QObject::connect(_settings.get(), &AppSettings::categoryRulesChanged,
                   _activityService.get(), [this]() { applyCategoryRules(); });

  // After the category rules: goals are measured through them, and the
  // connections below run after applyCategoryRules() for the same reason.
  _dailyGoals = std::make_unique<activity::GoalsModel>(_settings.get());
  _goalService =
      std::make_unique<activity::GoalService>(*_activityQueryService);
  _goalService->setGoals(_settings->dailyGoals());
  QObject::connect(
      _settings.get(), &AppSettings::dailyGoalsChanged, _goalService.get(),
      [this]() { _goalService->setGoals(_settings->dailyGoals()); });
  QObject::connect(_settings.get(), &AppSettings::categoryRulesChanged,
                   _goalService.get(), &activity::GoalService::refresh);
  // Also covers a clear and every log edit: both announce activityRecorded.
  QObject::connect(_activityService.get(),
                   &activity::ActivityService::activityRecorded,
                   _goalService.get(), &activity::GoalService::refresh);

  _goalDigest =
      std::make_unique<activity::GoalDigestService>(*_activityQueryService);
  _goalDigest->setLastSentDay(_settings->goalDigestLastDay());
  applyGoalDigestSettings();
  for (auto signal :
       {&AppSettings::goalDigestChanged, &AppSettings::scheduleChanged,
        &AppSettings::dailyGoalsChanged}) {
    QObject::connect(_settings.get(), signal, _goalDigest.get(),
                     [this]() { applyGoalDigestSettings(); });
  }
  QObject::connect(_goalDigest.get(),
                   &activity::GoalDigestService::lastSentDayChanged,
                   _settings.get(), &AppSettings::setGoalDigestLastDay);

  _goalController = std::make_unique<activity::GoalController>(
      _goalService.get(), _settings.get(), _tray.get(), _goalDigest.get());
  QObject::connect(_translations.get(), &TranslationManager::languageApplied,
                   _goalController.get(),
                   &activity::GoalController::retranslate);

  _activityService->setSchedule(_settings->schedule());
  _activityService->setTrackingEnabled(_settings->trackingEnabled());

  QObject::connect(_settings.get(), &AppSettings::trackingEnabledChanged,
                   _activityService.get(), [this]() {
                     _activityService->setTrackingEnabled(
                         _settings->trackingEnabled());
                   });
  QObject::connect(
      _settings.get(), &AppSettings::scheduleChanged, _activityService.get(),
      [this]() { _activityService->setSchedule(_settings->schedule()); });

  _activityController = std::make_unique<activity::UserActivityController>(
      _activityService.get(), _settings.get());

  _dayQuery = std::make_unique<activity::ActivityQueryController>(
      *_activityQueryService);
  _periodQuery = std::make_unique<activity::ActivityQueryController>(
      *_activityQueryService);
  _logQuery = std::make_unique<activity::ActivityQueryController>(
      *_activityQueryService);
  _monthQuery = std::make_unique<activity::ActivityQueryController>(
      *_activityQueryService);

  _periodQuery->applyPreset(QStringLiteral("last7days"));
  _logQuery->applyPreset(QStringLiteral("last7days"));
  _monthQuery->applyPreset(QStringLiteral("thismonth"));
  _monthQuery->setGranularity(QStringLiteral("day"));

  for (activity::ActivityQueryController *query :
       {_dayQuery.get(), _periodQuery.get(), _logQuery.get(),
        _monthQuery.get()}) {
    QObject::connect(_settings.get(), &AppSettings::categoryRulesChanged, query,
                     &activity::ActivityQueryController::refreshLater);
    QObject::connect(_activityService.get(),
                     &activity::ActivityService::activityRecorded, query,
                     &activity::ActivityQueryController::refreshLater);
    QObject::connect(_activityService.get(),
                     &activity::ActivityService::cleared, query,
                     &activity::ActivityQueryController::refreshLater);

    QObject::connect(_translations.get(), &TranslationManager::languageApplied,
                     query, &activity::ActivityQueryController::retranslate);
  }

  qCInfo(lcInit) << "Activity module wired";
}

void AppInitializer::applyCategoryRules() {
  const QList<activity::CategoryRule> &rules = _settings->categoryRules();
  _activityQueryService->setCategoryRules(activity::CategoryRules(rules));
  _activityService->setCategoryRules(rules);
}

void AppInitializer::applyGoalDigestSettings() {
  _goalDigest->setGoals(_settings->dailyGoals());
  _goalDigest->setPlan(_settings->digestPlan());
  _goalDigest->setSchedule(_settings->schedule());
  _goalDigest->setCatchUp(_settings->goalDigestCatchUp());
}

void AppInitializer::applyPrivacyRules() {
  _activityService->setPrivacyRules(_settings->privacyRules());
}

void AppInitializer::registerQmlTypes() {
  QQmlContext *context = _engine->rootContext();
  context->setContextProperty("activityController", _activityController.get());
  context->setContextProperty("appSettings", _settings.get());
  context->setContextProperty("settingsController", _settingsController.get());
  context->setContextProperty("dayQuery", _dayQuery.get());
  context->setContextProperty("periodQuery", _periodQuery.get());
  context->setContextProperty("logQuery", _logQuery.get());
  context->setContextProperty("monthQuery", _monthQuery.get());
  context->setContextProperty("categoryRules", _categoryRules.get());
  context->setContextProperty("privacyRules", _privacyRules.get());
  context->setContextProperty("dailyGoals", _dailyGoals.get());
  context->setContextProperty("goalController", _goalController.get());

  QObject::connect(
      _engine.get(), &QQmlApplicationEngine::objectCreationFailed, &_app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  _engine->loadFromModule("Chronexa", "Main");

  QObject::connect(_tray.get(), &system::TrayNotifier::openRequested, this,
                   &AppInitializer::showMainWindow);
  QObject::connect(_tray.get(), &system::TrayNotifier::quitRequested, this,
                   &AppInitializer::closeMainWindow);

  // Last, with the window and the tray up: the first look is where a summary
  // missed while the app was closed goes out.
  _goalDigest->start(kDigestFirstCheckMs);

  qCInfo(lcInit) << "QML types registered";
}

QWindow *AppInitializer::mainWindow() const {
  const QList<QObject *> roots = _engine->rootObjects();
  return roots.isEmpty() ? nullptr : qobject_cast<QWindow *>(roots.first());
}

void AppInitializer::showMainWindow() {
  QWindow *window = mainWindow();
  if (window == nullptr) {
    return;
  }
  // Only the minimised flag goes: a maximised window comes back maximised.
  window->setWindowStates(window->windowStates() & ~Qt::WindowMinimized);
  window->show();
  window->raise();
  window->requestActivate();
}

void AppInitializer::closeMainWindow() {
  QWindow *window = mainWindow();
  if (window == nullptr) {
    QCoreApplication::quit();
    return;
  }
  window->close();
}

} // namespace chronexa::core
