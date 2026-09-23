#pragma once

#include <QList>

#include <optional>

#include "domain/activity/Activity.hpp"

namespace chronexa::activity {

class IUserActivityProvider {
public:
  virtual ~IUserActivityProvider() = default;

  virtual void start() = 0;
  virtual void stop() = 0;
  virtual bool isRunning() const = 0;
  virtual QList<Activity> drainEvents() = 0;
  virtual std::optional<Activity> currentSession() const = 0;
  virtual bool isIdle() const = 0;
};

} // namespace chronexa::activity
