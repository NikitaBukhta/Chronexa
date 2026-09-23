#pragma once

#include <QString>

namespace chronexa::activity {

// One look at what the user is doing right now.
struct ForegroundSample {
  // The user has not touched keyboard or mouse for longer than the idle
  // threshold. appName and title are not read then.
  bool idle = false;
  // Empty when there is no foreground window, or its process is unreadable.
  QString appName;
  QString title;
};

// The OS-specific half of activity tracking: reads the foreground window and
// the idle state, and nothing else. What those samples add up to is decided
// by PollingActivityProvider, so the session rules can be tested against a
// fake probe.
class IForegroundProbe {
public:
  virtual ~IForegroundProbe() = default;

  virtual ForegroundSample sample() = 0;
};

} // namespace chronexa::activity
