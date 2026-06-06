#pragma once

#include <QObject>

namespace chronexa::activity {

class ActivityService;

class UserActivityController : public QObject {
  Q_OBJECT

public:
  explicit UserActivityController(ActivityService *service, QObject *parent = nullptr);

  Q_INVOKABLE void clearActivities();

private:
  ActivityService *_service;
};

} // namespace chronexa::activity
