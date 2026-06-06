#pragma once

#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>

#include <memory>

namespace chronexa::activity {
class ActivityService;
class UserActivityModel;
class UserActivityController;
} // namespace chronexa::activity

namespace chronexa::core {

class AppInitializer : public QObject {
  Q_OBJECT

public:
  explicit AppInitializer(QGuiApplication &app, QObject *parent = nullptr);
  ~AppInitializer() override;
  int run();

private:
  void init();
  void buildActivityModule();
  void registerQmlTypes();

private:
  QGuiApplication &_app;
  std::unique_ptr<QQmlApplicationEngine> _engine;
  std::unique_ptr<activity::ActivityService> _activityService;
  std::unique_ptr<activity::UserActivityModel> _activityModel;
  std::unique_ptr<activity::UserActivityController> _activityController;
};

} // namespace chronexa::core
