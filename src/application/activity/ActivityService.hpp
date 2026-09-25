#pragma once

#include "domain/activity/CategoryRules.hpp"
#include "domain/activity/PrivacyRules.hpp"
#include "domain/activity/TrackingSchedule.hpp"
#include "infrastructure/activity/IActivityRepository.hpp"
#include "infrastructure/activity/IUserActivityProvider.hpp"

#include <QList>
#include <QObject>

#include <memory>
#include <optional>

namespace chronexa::activity {

enum class EditResult {
  Done,
  Invalid,
  Excluded,
  Failed,
};

class ActivityService : public QObject {
  Q_OBJECT

public:
  explicit ActivityService(IActivityRepository &repository,
                           QObject *parent = nullptr);
  // For tests: runs on the given provider instead of the OS one.
  ActivityService(IActivityRepository &repository,
                  std::unique_ptr<IUserActivityProvider> provider,
                  QObject *parent = nullptr);
  ~ActivityService() override;

  void requestClear();

  void setTrackingEnabled(bool enabled);
  [[nodiscard]] bool isTrackingEnabled() const;

  void setSchedule(const TrackingSchedule &schedule);
  [[nodiscard]] TrackingSchedule schedule() const;

  void setCategoryRules(const QList<CategoryRule> &rules);
  void setPrivacyRules(const QList<PrivacyRule> &rules);

  int applyPrivacyToHistory();

  EditResult editSession(const Activity &session, const SessionEdit &edit);
  EditResult cutSession(const Activity &session, const QDateTime &from,
                        const QDateTime &to);

  [[nodiscard]] bool isTracking() const;
  [[nodiscard]] bool isHeldBySchedule() const;
  [[nodiscard]] QDateTime nextScheduleChange() const;
  [[nodiscard]] std::optional<Activity> currentSession() const;
  [[nodiscard]] bool isIdle() const;

signals:
  void activityRecorded();

  void currentActivityChanged();

  void trackingChanged();
  void cleared();

private slots:
  void onFlushTick();
  void onHeartbeat();

private:
  enum class Notify { No, Yes };

  void scheduleNextFlush();
  void flush(Notify notify);

  void applyTrackingState();
  void redactPending();

  IActivityRepository &_repository;
  std::unique_ptr<IUserActivityProvider> _activityProvider;

  QList<Activity> _pending;

  bool _trackingEnabled = true;
  TrackingSchedule _schedule;
  PrivacyRules _privacy;
};

} // namespace chronexa::activity
