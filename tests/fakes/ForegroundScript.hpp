#pragma once

#include "Fakes.hpp"
#include "infrastructure/activity/PollingActivityProvider.hpp"

#include <memory>

namespace chronexa::activity::testing {

// A scripted user in front of a PollingActivityProvider: owns the fake probe
// and clock, and polls once per simulated second. The provider itself is
// handed out, so it can be given to an ActivityService under test.
class ForegroundScript {
public:
  explicit ForegroundScript(const QDateTime &start = utc(10, 0))
      : _clock(std::make_unique<ManualClock>(start)) {
    auto probe = std::make_unique<FakeForegroundProbe>();
    _probe = probe.get();
    ManualClock *clock = _clock.get();
    _ownedProvider = std::make_unique<PollingActivityProvider>(
        std::move(probe), [clock]() { return clock->now(); });
    _provider = _ownedProvider.get();
  }

  PollingActivityProvider &provider() { return *_provider; }
  FakeForegroundProbe &probe() { return *_probe; }
  ManualClock &clock() { return *_clock; }

  // Transfers ownership, e.g. into an ActivityService; the script keeps
  // driving it.
  std::unique_ptr<PollingActivityProvider> takeProvider() {
    return std::move(_ownedProvider);
  }

  void pollFor(int seconds) {
    for (int i = 0; i < seconds; ++i) {
      _provider->poll();
      _clock->advance(1);
    }
  }

  // Shows a window, then polls once a second for the given duration.
  void hold(const QString &app, const QString &title, int seconds) {
    _probe->show(app, title);
    pollFor(seconds);
  }

  void idleFor(int seconds) {
    _probe->goIdle();
    pollFor(seconds);
  }

  // No foreground window any more; the next poll closes the open session.
  void leave() {
    _probe->noWindow();
    _provider->poll();
  }

private:
  std::unique_ptr<ManualClock> _clock;
  FakeForegroundProbe *_probe = nullptr;
  std::unique_ptr<PollingActivityProvider> _ownedProvider;
  PollingActivityProvider *_provider = nullptr;
};

} // namespace chronexa::activity::testing
