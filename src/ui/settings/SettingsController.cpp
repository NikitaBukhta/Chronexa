#include "SettingsController.hpp"

#include "core/AppSettings.hpp"
#include "core/TranslationManager.hpp"
#include "domain/activity/TrackingSchedule.hpp"
#include "infrastructure/system/IAutoStartService.hpp"

#include <QLocale>
#include <QLoggingCategory>
#include <QRegularExpression>
#include <QVariantMap>

namespace {

Q_LOGGING_CATEGORY(lcSettings, "chronexa.settings.controller")

struct LanguageEntry {
  const char *code;
  const char *label;
};

constexpr LanguageEntry kLanguages[] = {
    {"system", nullptr},
    {"en", "English"},
    {"uk", "Українська"},
    {"ru", "Русский"},
};

} // namespace

namespace chronexa::settings {

SettingsController::SettingsController(core::AppSettings *settings,
                                       system::IAutoStartService *autoStart,
                                       core::TranslationManager *translations,
                                       QObject *parent)
    : QObject(parent), _settings(settings), _autoStart(autoStart),
      _translations(translations) {
  if (_settings != nullptr) {
    connect(_settings, &core::AppSettings::scheduleChanged, this,
            &SettingsController::scheduleSummaryChanged);
    connect(_settings, &core::AppSettings::languageChanged, this,
            &SettingsController::applyStoredLanguage);
  }

  if (_translations != nullptr) {
    connect(_translations, &core::TranslationManager::languageApplied, this,
            &SettingsController::languageChanged);
    connect(_translations, &core::TranslationManager::languageApplied, this,
            &SettingsController::scheduleSummaryChanged);
  }
}

bool SettingsController::autoStartSupported() const {
  return _autoStart != nullptr && _autoStart->isSupported();
}

bool SettingsController::autoStart() const {
  return _autoStart != nullptr && _autoStart->isEnabled();
}

void SettingsController::setAutoStart(bool enabled) {
  if (_autoStart == nullptr || enabled == _autoStart->isEnabled()) {
    return;
  }

  if (!_autoStart->setEnabled(enabled)) {
    qCWarning(lcSettings) << "Could not change the launch-at-sign-in setting";
    emit failed(tr("Could not change the launch-at-sign-in setting."));
  }

  emit autoStartChanged();
}

QVariantList SettingsController::languages() const {
  QVariantList entries;
  for (const LanguageEntry &language : kLanguages) {
    QVariantMap entry;
    entry[QStringLiteral("code")] = QLatin1String(language.code);
    entry[QStringLiteral("label")] = language.label == nullptr
                                         ? tr("System")
                                         : QString::fromUtf8(language.label);
    entries.append(entry);
  }
  return entries;
}

QString SettingsController::resolvedLanguage() const {
  return _translations != nullptr ? _translations->resolvedLanguage()
                                  : QString();
}

void SettingsController::applyStoredLanguage() {
  if (_settings == nullptr || _translations == nullptr) {
    return;
  }
  _translations->apply(_settings->language());
}

QStringList SettingsController::weekdayLabels() const {
  const QLocale locale;
  QStringList labels;
  labels.reserve(7);
  for (int day = 1; day <= 7; ++day) {
    labels.append(locale.dayName(day, QLocale::ShortFormat));
  }
  return labels;
}

QString SettingsController::formatMinutes(int minutes) const {
  const int clamped = qBound(0, minutes, 24 * 60 - 1);
  return QStringLiteral("%1:%2")
      .arg(clamped / 60, 2, 10, QLatin1Char('0'))
      .arg(clamped % 60, 2, 10, QLatin1Char('0'));
}

int SettingsController::parseMinutes(const QString &text) const {
  static const QRegularExpression pattern(
      QStringLiteral("^\\s*(\\d{1,2})\\D?(\\d{2})\\s*$"));

  const QRegularExpressionMatch match = pattern.match(text);
  if (!match.hasMatch()) {
    return -1;
  }

  const int hours = match.captured(1).toInt();
  const int minutes = match.captured(2).toInt();
  if (hours > 23 || minutes > 59) {
    return -1;
  }
  return hours * 60 + minutes;
}

void SettingsController::toggleScheduleDay(int dayOfWeek) {
  if (_settings == nullptr || dayOfWeek < 1 || dayOfWeek > 7) {
    return;
  }
  _settings->setScheduleDays(_settings->scheduleDays() ^
                             (1 << (dayOfWeek - 1)));
}

bool SettingsController::scheduleIncludesDay(int dayOfWeek) const {
  if (_settings == nullptr || dayOfWeek < 1 || dayOfWeek > 7) {
    return false;
  }
  return (_settings->scheduleDays() & (1 << (dayOfWeek - 1))) != 0;
}

void SettingsController::setScheduleDaysPreset(const QString &preset) {
  if (_settings == nullptr) {
    return;
  }
  if (preset == QStringLiteral("workdays")) {
    _settings->setScheduleDays(activity::TrackingSchedule::kWorkdays);
  } else if (preset == QStringLiteral("everyday")) {
    _settings->setScheduleDays(activity::TrackingSchedule::kEveryDay);
  }
}

QString SettingsController::scheduleSummary() const {
  if (_settings == nullptr) {
    return {};
  }
  if (!_settings->scheduleEnabled()) {
    return tr("Recording whenever tracking is on");
  }

  const int days = _settings->scheduleDays();
  if (days == activity::TrackingSchedule::kNoDays) {
    return tr("No day selected, so nothing is recorded");
  }

  QString dayText;
  if (days == activity::TrackingSchedule::kEveryDay) {
    dayText = tr("every day");
  } else if (days == activity::TrackingSchedule::kWorkdays) {
    dayText = tr("Mon–Fri");
  } else {
    const QStringList labels = weekdayLabels();
    QStringList selected;
    for (int day = 1; day <= 7; ++day) {
      if (scheduleIncludesDay(day)) {
        selected.append(labels.at(day - 1));
      }
    }
    dayText = selected.join(QStringLiteral(", "));
  }

  const QString window = QStringLiteral("%1 – %2").arg(
      formatMinutes(_settings->scheduleStartMinutes()),
      formatMinutes(_settings->scheduleEndMinutes()));

  if (_settings->scheduleEndMinutes() < _settings->scheduleStartMinutes()) {
    return tr("%1 overnight · %2").arg(window, dayText);
  }
  return QStringLiteral("%1 · %2").arg(window, dayText);
}

} // namespace chronexa::settings
