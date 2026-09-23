#include "AppSettings.hpp"

#include "AppEnvironment.hpp"

#include <QLoggingCategory>

namespace {

Q_LOGGING_CATEGORY(lcSettings, "chronexa.core.settings")

constexpr auto kTrackingEnabled = "tracking/enabled";
constexpr auto kScheduleEnabled = "tracking/scheduleEnabled";
constexpr auto kScheduleStart = "tracking/scheduleStartMinutes";
constexpr auto kScheduleEnd = "tracking/scheduleEndMinutes";
constexpr auto kScheduleDays = "tracking/scheduleDays";
constexpr auto kLanguage = "general/language";

constexpr int kMinutesPerDay = 24 * 60;

int clampMinutes(int minutes) { return qBound(0, minutes, kMinutesPerDay - 1); }

QTime timeFromMinutes(int minutes) {
  const int clamped = clampMinutes(minutes);
  return QTime(clamped / 60, clamped % 60);
}

int minutesFromTime(const QTime &time) {
  return time.isValid() ? time.hour() * 60 + time.minute() : 0;
}

} // namespace

namespace chronexa::core {

AppSettings::AppSettings(QObject *parent)
    : QObject(parent), _store(std::make_unique<QSettings>()) {
  _trackingEnabled = _store->value(kTrackingEnabled, true).toBool();

  _schedule.enabled = _store->value(kScheduleEnabled, false).toBool();
  _schedule.start = timeFromMinutes(
      _store->value(kScheduleStart, minutesFromTime(QTime(9, 0))).toInt());
  _schedule.end = timeFromMinutes(
      _store->value(kScheduleEnd, minutesFromTime(QTime(18, 0))).toInt());
  _schedule.days =
      _store->value(kScheduleDays, activity::TrackingSchedule::kWorkdays)
          .toInt() &
      activity::TrackingSchedule::kEveryDay;

  _language = _store->value(kLanguage, QStringLiteral("system")).toString();

  qCInfo(lcSettings) << "Settings loaded from" << _store->fileName();
}

AppSettings::~AppSettings() = default;

void AppSettings::store(const QString &key, const QVariant &value) {
  _store->setValue(key, value);
  _store->sync();
}

bool AppSettings::trackingEnabled() const { return _trackingEnabled; }

void AppSettings::setTrackingEnabled(bool enabled) {
  if (_trackingEnabled == enabled) {
    return;
  }
  _trackingEnabled = enabled;
  store(QLatin1String(kTrackingEnabled), enabled);
  emit trackingEnabledChanged();
}

bool AppSettings::scheduleEnabled() const { return _schedule.enabled; }

void AppSettings::setScheduleEnabled(bool enabled) {
  if (_schedule.enabled == enabled) {
    return;
  }
  _schedule.enabled = enabled;
  store(QLatin1String(kScheduleEnabled), enabled);
  emit scheduleChanged();
}

int AppSettings::scheduleStartMinutes() const {
  return minutesFromTime(_schedule.start);
}

void AppSettings::setScheduleStartMinutes(int minutes) {
  const QTime time = timeFromMinutes(minutes);
  if (_schedule.start == time) {
    return;
  }
  _schedule.start = time;
  store(QLatin1String(kScheduleStart), minutesFromTime(time));
  emit scheduleChanged();
}

int AppSettings::scheduleEndMinutes() const {
  return minutesFromTime(_schedule.end);
}

void AppSettings::setScheduleEndMinutes(int minutes) {
  const QTime time = timeFromMinutes(minutes);
  if (_schedule.end == time) {
    return;
  }
  _schedule.end = time;
  store(QLatin1String(kScheduleEnd), minutesFromTime(time));
  emit scheduleChanged();
}

int AppSettings::scheduleDays() const { return _schedule.days; }

void AppSettings::setScheduleDays(int days) {
  const int masked = days & activity::TrackingSchedule::kEveryDay;
  if (_schedule.days == masked) {
    return;
  }
  _schedule.days = masked;
  store(QLatin1String(kScheduleDays), masked);
  emit scheduleChanged();
}

QString AppSettings::language() const { return _language; }

void AppSettings::setLanguage(const QString &language) {
  if (_language == language || language.isEmpty()) {
    return;
  }
  _language = language;
  store(QLatin1String(kLanguage), language);
  emit languageChanged();
}

QString AppSettings::historyPath() const {
  return AppEnvironment::databasePath();
}

activity::TrackingSchedule AppSettings::schedule() const { return _schedule; }

} // namespace chronexa::core
