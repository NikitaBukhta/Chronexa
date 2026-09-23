#pragma once

#include "domain/activity/CategoryRules.hpp"
#include "domain/activity/TrackingSchedule.hpp"
#include "infrastructure/activity/IActivityRepository.hpp"
#include "infrastructure/activity/IUserActivityProvider.hpp"

#include <QList>
#include <QObject>

#include <memory>
#include <optional>

namespace chronexa::activity {

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
  bool isTrackingEnabled() const;

  void setSchedule(const TrackingSchedule &schedule);
  TrackingSchedule schedule() const;

  // Sessions are split where the category changes, so a browser session that
  // moves from a work tab to YouTube is recorded as two.
  void setCategoryRules(const QList<CategoryRule> &rules);

  bool isTracking() const;

  bool isHeldBySchedule() const;

  QDateTime nextScheduleChange() const;

  std::optional<Activity> currentSession() const;
  bool isIdle() const;

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

  IActivityRepository &_repository;
  std::unique_ptr<IUserActivityProvider> _activityProvider;

  // Drained from the provider but not yet accepted by the repository.
  QList<Activity> _pending;

  bool _trackingEnabled = true;
  TrackingSchedule _schedule;
};

} // namespace chronexa::activity
