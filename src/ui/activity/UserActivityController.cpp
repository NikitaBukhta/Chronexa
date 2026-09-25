#include "UserActivityController.hpp"

#include "ActivityFormat.hpp"
#include "application/activity/ActivityService.hpp"
#include "core/AppSettings.hpp"

#include <QLoggingCategory>

namespace {

Q_LOGGING_CATEGORY(lcController, "chronexa.activity.controller")

} // namespace

namespace chronexa::activity {

UserActivityController::UserActivityController(ActivityService *service,
                                               core::AppSettings *settings,
                                               QObject *parent)
    : QObject(parent), _service(service), _settings(settings) {
  if (_service != nullptr) {
    connect(_service, &ActivityService::currentActivityChanged, this,
            &UserActivityController::currentActivityChanged);
    connect(_service, &ActivityService::trackingChanged, this,
            &UserActivityController::trackingChanged);
    connect(_service, &ActivityService::activityRecorded, this,
            &UserActivityController::historyChanged);
    connect(_service, &ActivityService::cleared, this,
            &UserActivityController::historyChanged);
  }

  if (_settings != nullptr) {
    connect(_settings, &core::AppSettings::trackingEnabledChanged, this,
            &UserActivityController::trackingChanged);
    connect(_settings, &core::AppSettings::scheduleChanged, this,
            &UserActivityController::trackingChanged);
  }
}

bool UserActivityController::isTrackingEnabled() const {
  return _settings != nullptr && _settings->trackingEnabled();
}

void UserActivityController::setTrackingEnabled(bool enabled) {
  if (_settings != nullptr) {
    _settings->setTrackingEnabled(enabled);
  }
}

void UserActivityController::toggleTracking() {
  setTrackingEnabled(!isTrackingEnabled());
}

bool UserActivityController::isTracking() const {
  return _service != nullptr && _service->isTracking();
}

bool UserActivityController::isHeldBySchedule() const {
  return _service != nullptr && _service->isHeldBySchedule();
}

QString UserActivityController::scheduleHoldText() const {
  if (!isHeldBySchedule()) {
    return {};
  }

  const QDateTime resumesAt = _service->nextScheduleChange();
  if (!resumesAt.isValid()) {
    return tr("No day is selected in the schedule");
  }

  const QDate today = QDate::currentDate();
  if (resumesAt.date() == today) {
    return tr("Resumes at %1").arg(format::clock(resumesAt));
  }
  return tr("Resumes %1 at %2")
      .arg(format::dayLabel(resumesAt.date()), format::clock(resumesAt));
}

bool UserActivityController::isIdle() const {
  return _service != nullptr && _service->isIdle();
}

bool UserActivityController::hasCurrent() const {
  return _service != nullptr && _service->currentSession().has_value();
}

QString UserActivityController::currentAppName() const {
  if (_service == nullptr) {
    return {};
  }
  const auto session = _service->currentSession();
  return session ? session->appName : QString();
}

QString UserActivityController::currentTitle() const {
  if (_service == nullptr) {
    return {};
  }
  const auto session = _service->currentSession();
  return session ? session->title : QString();
}

qint64 UserActivityController::currentSeconds() const {
  if (_service == nullptr) {
    return 0;
  }
  const auto session = _service->currentSession();
  return session ? session->durationSeconds() : 0;
}

QString UserActivityController::currentSecondsText() const {
  return format::stopwatch(currentSeconds());
}

void UserActivityController::clearActivities() {
  qCInfo(lcController) << "clearActivities() invoked";
  if (_service != nullptr) {
    _service->requestClear();
  }
}

int UserActivityController::applyPrivacyToHistory() {
  qCInfo(lcController) << "applyPrivacyToHistory() invoked";
  return _service != nullptr ? _service->applyPrivacyToHistory() : -1;
}

namespace {

Activity sessionOf(const QString &appName, const QString &title,
                   const QDateTime &from, const QDateTime &to) {
  Activity session;
  session.appName = appName;
  session.title = title;
  session.startedOn = from;
  session.endedOn = to;
  return session;
}

} // namespace

QString UserActivityController::editResultText(EditResult result) const {
  switch (result) {
  case EditResult::Done:
    return {};
  case EditResult::Invalid:
    return tr("Enter the application name.");
  case EditResult::Excluded:
    return tr("Your privacy rules exclude this window, so it cannot be "
              "stored.");
  case EditResult::Failed:
    break;
  }
  return tr("The history could not be changed.");
}

QString UserActivityController::editSession(
    const QString &appName, const QString &title, const QDateTime &from,
    const QDateTime &to, const QString &newAppName, const QString &newTitle,
    const QString &categoryMode, const QString &category) {
  if (_service == nullptr) {
    return editResultText(EditResult::Failed);
  }

  SessionEdit edit;
  edit.appName = newAppName;
  edit.title = newTitle;
  if (categoryMode == QStringLiteral("none")) {
    edit.category = QStringLiteral("");
  } else if (categoryMode == QStringLiteral("set")) {
    edit.category = category;
  }
  return editResultText(
      _service->editSession(sessionOf(appName, title, from, to), edit));
}

QString UserActivityController::cutSession(
    const QString &appName, const QString &title, const QDateTime &from,
    const QDateTime &to, const QDateTime &cutFrom, const QDateTime &cutTo) {
  if (_service == nullptr) {
    return editResultText(EditResult::Failed);
  }
  const EditResult result =
      _service->cutSession(sessionOf(appName, title, from, to), cutFrom, cutTo);
  if (result == EditResult::Invalid) {
    return tr("Choose a time inside the session.");
  }
  return editResultText(result);
}

QString UserActivityController::formatDuration(qint64 seconds,
                                               bool compact) const {
  return format::duration(seconds, compact);
}

QString UserActivityController::formatClock(const QDateTime &moment) const {
  return format::clock(moment);
}

QString UserActivityController::formatDayLabel(const QDate &date) const {
  return format::dayLabel(date);
}

} // namespace chronexa::activity
