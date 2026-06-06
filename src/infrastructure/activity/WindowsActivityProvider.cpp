#include "WindowsActivityProvider.hpp"

#include <QMutexLocker>

namespace chronexa::activity {

WindowsActivityProvider::WindowsActivityProvider() {}

WindowsActivityProvider::~WindowsActivityProvider() {}

void WindowsActivityProvider::start() {}

void WindowsActivityProvider::stop() {}

QList<Activity> WindowsActivityProvider::drainEvents() {
  QMutexLocker locker(&_bufferMutex);
  return std::move(_buffer);
}

} // namespace chronexa::activity