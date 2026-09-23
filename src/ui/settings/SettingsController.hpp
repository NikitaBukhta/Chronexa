#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>

namespace chronexa::core {

class AppSettings;
class TranslationManager;

} // namespace chronexa::core

namespace chronexa::system {

class IAutoStartService;

} // namespace chronexa::system

namespace chronexa::settings {

class SettingsController : public QObject {
  Q_OBJECT

  Q_PROPERTY(
      bool autoStart READ autoStart WRITE setAutoStart NOTIFY autoStartChanged)
  Q_PROPERTY(bool autoStartSupported READ autoStartSupported CONSTANT)

  // Not CONSTANT: the "System" entry is a translated label.
  Q_PROPERTY(QVariantList languages READ languages NOTIFY languageChanged)

  Q_PROPERTY(
      QString resolvedLanguage READ resolvedLanguage NOTIFY languageChanged)

  Q_PROPERTY(
      QStringList weekdayLabels READ weekdayLabels NOTIFY languageChanged)

  Q_PROPERTY(QString scheduleSummary READ scheduleSummary NOTIFY
                 scheduleSummaryChanged)

public:
  SettingsController(core::AppSettings *settings,
                     system::IAutoStartService *autoStart,
                     core::TranslationManager *translations,
                     QObject *parent = nullptr);

  bool autoStart() const;
  void setAutoStart(bool enabled);
  bool autoStartSupported() const;

  QVariantList languages() const;
  QString resolvedLanguage() const;
  QStringList weekdayLabels() const;
  QString scheduleSummary() const;

  void applyStoredLanguage();

  Q_INVOKABLE QString formatMinutes(int minutes) const;

  Q_INVOKABLE int parseMinutes(const QString &text) const;

  Q_INVOKABLE void toggleScheduleDay(int dayOfWeek);
  Q_INVOKABLE bool scheduleIncludesDay(int dayOfWeek) const;

  Q_INVOKABLE void setScheduleDaysPreset(const QString &preset);

signals:
  void autoStartChanged();
  void languageChanged();
  void scheduleSummaryChanged();

  void failed(const QString &message);

private:
  core::AppSettings *_settings;
  system::IAutoStartService *_autoStart;
  core::TranslationManager *_translations;
};

} // namespace chronexa::settings
