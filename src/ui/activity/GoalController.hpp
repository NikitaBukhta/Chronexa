#pragma once

#include "domain/activity/DailyGoals.hpp"
#include "ui/activity/GoalProgressModel.hpp"

#include <QObject>
#include <QString>

namespace chronexa::core {

class AppSettings;

} // namespace chronexa::core

namespace chronexa::system {

class INotifier;

} // namespace chronexa::system

namespace chronexa::activity {

class GoalDigestService;
class GoalService;
struct GoalDigest;

// Today's goals for QML, and the tray message when one is crossed.
class GoalController : public QObject {
  Q_OBJECT

  Q_PROPERTY(
      chronexa::activity::GoalProgressModel *progress READ progress CONSTANT)
  Q_PROPERTY(bool hasGoals READ hasGoals NOTIFY progressChanged)
  Q_PROPERTY(int onTrackCount READ onTrackCount NOTIFY progressChanged)
  Q_PROPERTY(QString summary READ summary NOTIFY progressChanged)
  Q_PROPERTY(bool notificationsSupported READ notificationsSupported CONSTANT)
  Q_PROPERTY(QString nextDigestText READ nextDigestText NOTIFY digestChanged)

public:
  GoalController(GoalService *service, core::AppSettings *settings,
                 system::INotifier *notifier,
                 GoalDigestService *digest = nullptr,
                 QObject *parent = nullptr);

  [[nodiscard]] GoalProgressModel *progress() const;
  [[nodiscard]] bool hasGoals() const;
  // Limits still kept plus targets already reached.
  [[nodiscard]] int onTrackCount() const;
  [[nodiscard]] QString summary() const;
  [[nodiscard]] bool notificationsSupported() const;
  [[nodiscard]] QString nextDigestText() const;

  void retranslate();

signals:
  void progressChanged();
  void digestChanged();

private:
  void onProgressChanged();
  void onGoalCrossed(const GoalProgress &progress);
  void onDigestDue(const GoalDigest &digest);
  void notify(const QString &title, const QString &message);
  void updateColorOrder();

  GoalService *_service = nullptr;
  core::AppSettings *_settings = nullptr;
  system::INotifier *_notifier = nullptr;
  GoalDigestService *_digest = nullptr;
  GoalProgressModel *_progress = nullptr;
};

} // namespace chronexa::activity
