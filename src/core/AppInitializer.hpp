#pragma once

#include <QGuiApplication>
#include <QObject>
#include <QQmlApplicationEngine>

#include <memory>

namespace chronexa::activity {

class ActivityQueryController;
class ActivityQueryService;
class ActivityService;
class CategoryRulesModel;
class PrivacyRulesModel;
class SqliteActivityRepository;
class UserActivityController;

} // namespace chronexa::activity

namespace chronexa::settings {

class SettingsController;

} // namespace chronexa::settings

namespace chronexa::system {

class IAutoStartService;

} // namespace chronexa::system

namespace chronexa::core {

class AppSettings;
class TranslationManager;

class AppInitializer : public QObject {
  Q_OBJECT

public:
  explicit AppInitializer(QGuiApplication &app, QObject *parent = nullptr);
  ~AppInitializer() override;
  int run();

private:
  void init();
  void applyCommandLine();
  void buildSettingsModule();
  void buildActivityModule();
  void applyCategoryRules();
  void applyPrivacyRules();
  void registerQmlTypes();
  void shutdown();

private:
  QGuiApplication &_app;
  std::unique_ptr<QQmlApplicationEngine> _engine;

  std::unique_ptr<AppSettings> _settings;
  std::unique_ptr<TranslationManager> _translations;
  std::unique_ptr<system::IAutoStartService> _autoStart;
  std::unique_ptr<settings::SettingsController> _settingsController;

  std::unique_ptr<activity::SqliteActivityRepository> _activityRepository;
  std::unique_ptr<activity::ActivityQueryService> _activityQueryService;
  std::unique_ptr<activity::ActivityService> _activityService;
  std::unique_ptr<activity::UserActivityController> _activityController;
  std::unique_ptr<activity::CategoryRulesModel> _categoryRules;
  std::unique_ptr<activity::PrivacyRulesModel> _privacyRules;

  std::unique_ptr<activity::ActivityQueryController> _dayQuery;
  std::unique_ptr<activity::ActivityQueryController> _periodQuery;
  std::unique_ptr<activity::ActivityQueryController> _logQuery;

  std::unique_ptr<activity::ActivityQueryController> _monthQuery;
};

} // namespace chronexa::core
