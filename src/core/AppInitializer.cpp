#include "AppInitializer.hpp"
#include "AppEnvironment.hpp"

#include "application/activity/ActivityService.hpp"
#include "ui/activity/UserActivityController.hpp"
#include "ui/activity/UserActivityModel.hpp"

#include <QLoggingCategory>
#include <QQmlContext>

namespace {

Q_LOGGING_CATEGORY(lcInit, "chronexa.core.init")

}

namespace chronexa::core {

AppInitializer::AppInitializer(QGuiApplication &app, QObject *parent)
    : QObject(parent), _app(app),
      _engine(std::make_unique<QQmlApplicationEngine>()) {}

AppInitializer::~AppInitializer() = default;

int AppInitializer::run() {
  init();
  const int res = QGuiApplication::exec();
  _engine.reset();
  AppEnvironment::shutdownFileLogger();
  return res;
}

void AppInitializer::init() {
  AppEnvironment::installFileLogger();

#ifdef QT_NO_DEBUG
  QLoggingCategory::setFilterRules("chronexa.*.debug=false\n"
                                   "chronexa.*.info=false");
#else
  QLoggingCategory::setFilterRules("chronexa.*.debug=true");
#endif

  buildActivityModule();
  registerQmlTypes();
}

void AppInitializer::buildActivityModule() {
  _activityService = std::make_unique<activity::ActivityService>();
  _activityModel = std::make_unique<activity::UserActivityModel>();
  _activityController = std::make_unique<activity::UserActivityController>(
      _activityService.get());

  QObject::connect(
      _activityService.get(), &activity::ActivityService::activityUpdated,
      _activityModel.get(), &activity::UserActivityModel::onActivitiesReceived);
  QObject::connect(_activityService.get(), &activity::ActivityService::cleared,
                   _activityModel.get(), &activity::UserActivityModel::clear);

  qCInfo(lcInit) << "Activity module wired";
}

void AppInitializer::registerQmlTypes() {
  _engine->rootContext()->setContextProperty("activityModel",
                                             _activityModel.get());
  _engine->rootContext()->setContextProperty("activityController",
                                             _activityController.get());

  QObject::connect(
      _engine.get(), &QQmlApplicationEngine::objectCreationFailed, &_app,
      []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

  _engine->loadFromModule("Chronexa", "Main");

  qCInfo(lcInit) << "QML types registered";
}

} // namespace chronexa::core
