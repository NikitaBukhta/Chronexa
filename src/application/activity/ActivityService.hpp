#pragma once

#include "infrastructure/activity/IUserActivityProvider.hpp"

#include <QObject>

namespace chronexa::activity {

class ActivityService : public QObject {
  Q_OBJECT

public:
  explicit ActivityService(QObject *parent = nullptr);
  ~ActivityService() override;

  void requestClear();

signals:
  void activityUpdated(const QList<Activity> &activities);
  void cleared();

private slots:
  void onTick();

private:
  void scheduleNextTick();

  // Non-owning: the OS-specific provider is a process-wide singleton.
  IUserActivityProvider *_activityProvider = nullptr;
};

} // namespace chronexa::activity
