#include "GoalController.hpp"
#include "ActivityFormat.hpp"

#include "application/activity/GoalDigestService.hpp"
#include "application/activity/GoalService.hpp"
#include "core/AppSettings.hpp"
#include "infrastructure/system/INotifier.hpp"

#include <QLoggingCategory>

#include <algorithm>

namespace {

Q_LOGGING_CATEGORY(lcGoalUi, "chronexa.activity.goalui")

} // namespace

namespace chronexa::activity {

GoalController::GoalController(GoalService *service,
                               core::AppSettings *settings,
                               system::INotifier *notifier,
                               GoalDigestService *digest, QObject *parent)
    : QObject(parent), _service(service), _settings(settings),
      _notifier(notifier), _digest(digest),
      _progress(new GoalProgressModel(this)) {
  updateColorOrder();
  _progress->setProgress(_service->progress());

  connect(_service, &GoalService::progressChanged, this,
          &GoalController::onProgressChanged);
  connect(_service, &GoalService::goalCrossed, this,
          &GoalController::onGoalCrossed);
  connect(_settings, &core::AppSettings::categoryRulesChanged, this,
          &GoalController::updateColorOrder);
  if (_digest != nullptr) {
    connect(_digest, &GoalDigestService::digestDue, this,
            &GoalController::onDigestDue);
    connect(_digest, &GoalDigestService::nextDueChanged, this,
            &GoalController::digestChanged);
  }
}

GoalProgressModel *GoalController::progress() const { return _progress; }

bool GoalController::hasGoals() const {
  return !_service->progress().isEmpty();
}

int GoalController::onTrackCount() const {
  const QList<GoalProgress> &progress = _service->progress();
  return static_cast<int>(std::count_if(
      progress.cbegin(), progress.cend(), [](const GoalProgress &item) {
        const GoalState state = item.state();
        return state == GoalState::Within || state == GoalState::Reached;
      }));
}

QString GoalController::summary() const {
  return tr("%1 of %2 on track")
      .arg(onTrackCount())
      .arg(_service->progress().size());
}

bool GoalController::notificationsSupported() const {
  return _notifier != nullptr && _notifier->isSupported();
}

QString GoalController::nextDigestText() const {
  if (_digest == nullptr) {
    return {};
  }
  return format::nextDigest(_digest->nextDue(), _settings->digestPlan().when);
}

void GoalController::retranslate() {
  _progress->retranslate();
  emit progressChanged();
  emit digestChanged();
}

void GoalController::onProgressChanged() {
  _progress->setProgress(_service->progress());
  emit progressChanged();
}

void GoalController::onGoalCrossed(const GoalProgress &progress) {
  if (!_settings->goalNotifications()) {
    qCDebug(lcGoalUi) << "Goal crossed, notifications off:"
                      << progress.goal.category;
    return;
  }
  notify(format::goalAlertTitle(progress), format::goalAlertMessage(progress));
}

void GoalController::onDigestDue(const GoalDigest &digest) {
  if (!_settings->goalNotifications()) {
    qCDebug(lcGoalUi) << "Summary due, notifications off";
    return;
  }
  notify(format::digestTitle(digest), format::digestMessage(digest));
}

void GoalController::notify(const QString &title, const QString &message) {
  if (_notifier != nullptr) {
    _notifier->notify(title, message);
  }
}

void GoalController::updateColorOrder() {
  _progress->setColorOrder(
      CategoryRules(_settings->categoryRules()).categoryNames());
}

} // namespace chronexa::activity
