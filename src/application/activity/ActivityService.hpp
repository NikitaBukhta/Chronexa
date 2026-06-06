#pragma once

#include "infrastructure/activity/IUserActivityProvider.hpp"

#include <QObject>

#include <memory>

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

  std::unique_ptr<IUserActivityProvider> _activityProvider;
};

} // namespace chronexa::activity
