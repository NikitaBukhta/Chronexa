#pragma once

#include <QList>

#include "domain/activity/Activity.hpp"

namespace chronexa::activity {

class IUserActivityProvider {
public:
  virtual ~IUserActivityProvider() = default;

  virtual void start() = 0;
  virtual void stop() = 0;

  virtual QList<Activity> drainEvents() = 0;
};

} // namespace chronexa::activity
