#pragma once

#include "IUserActivityProvider.hpp"

#include <QList>
#include <QMutex>

namespace chronexa::activity {

class WindowsActivityProvider : public IUserActivityProvider {
public:
  WindowsActivityProvider();
  ~WindowsActivityProvider() override;

  void start() override;
  void stop() override;
  QList<Activity> drainEvents() override;

private:
  QList<Activity> _buffer;
  QMutex _bufferMutex;
};

} // namespace chronexa::activity