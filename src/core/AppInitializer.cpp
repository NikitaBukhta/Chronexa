#include "AppInitializer.hpp"
#include "AppEnvironment.hpp"
#include "AppSettings.hpp"
#include "TranslationManager.hpp"

#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/ActivityService.hpp"
#include "infrastructure/activity/SqliteActivityRepository.hpp"
#include "ui/activity/ActivityQueryController.hpp"
#include "ui/activity/CategoryRulesModel.hpp"
#include "ui/activity/UserActivityController.hpp"
#include "ui/settings/SettingsController.hpp"

#include <QLoggingCategory>
#include <QQmlContext>
#include <QQuickStyle>
#include <QtSystemDetection>

#ifdef Q_OS_WIN
#include "infrastructure/system/WindowsAutoStartService.hpp"
using OSSpecificAutoStart = chronexa::system::WindowsAutoStartService;
#else
#error "Unsupported platform"
#endif

namespace {

Q_LOGGING_CATEGORY(lcInit, "chronexa.core.init")

} // namespace

namespace chronexa::core {

AppInitializer::AppInitializer(QGuiApplication &app, QObject *parent)
    : QObject(parent), _app(app),
      _engine(std::make_unique<QQmlApplicationEngine>()) {}

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
  _categoryRules.reset();
  _activityController.reset();
  _activityService.reset();
  _activityQueryService.reset();
  _activityRepository.reset();

  _settingsController.reset();
  _autoStart.reset();
  _translations.reset();
  _settings.reset();

  qCInfo(lcInit) << "Shut down cleanly";
  AppEnvironment::shutdownFileLogger();
}

void AppInitializer::init() {
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

  // Seeds the default rules on first run, so it comes before anything reads
  // them. Built after the settings module: the default names are translated.
  _categoryRules =
      std::make_unique<activity::CategoryRulesModel>(_settings.get());
  applyCategoryRules();
  QObject::connect(_settings.get(), &AppSettings::categoryRulesChanged,
                   _activityService.get(), [this]() { applyCategoryRules(); });

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

  QObject::connect(
      _engine.get(), &QQmlApplicationEngine::objectCreationFailed, &_app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  _engine->loadFromModule("Chronexa", "Main");

  qCInfo(lcInit) << "QML types registered";
}

} // namespace chronexa::core
