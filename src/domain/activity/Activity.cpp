#include "Activity.hpp"

namespace chronexa::activity {

bool Activity::isValid() const {
  return !appName.isEmpty() && startedOn.isValid() && endedOn.isValid() &&
         endedOn > startedOn;
}

qint64 Activity::durationSeconds() const {
  if (!startedOn.isValid() || !endedOn.isValid()) {
    return 0;
  }
  return qMax<qint64>(0, startedOn.secsTo(endedOn));
}

} // namespace chronexa::activity
