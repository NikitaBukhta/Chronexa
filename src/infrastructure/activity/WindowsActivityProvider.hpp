#pragma once

#include "BrowserInfoParser.hpp"
#include "IUserActivityProvider.hpp"
#include "Win32Raii.hpp"
#include "domain/activity/ActivityCollector.hpp"

#include <QList>

namespace chronexa::activity {

class WindowsActivityProvider : public IUserActivityProvider {
public:
  WindowsActivityProvider(const WindowsActivityProvider &) = delete;
  ~WindowsActivityProvider() override;
  WindowsActivityProvider &operator=(const WindowsActivityProvider &) = delete;

  static WindowsActivityProvider *instance();

  void start() override;
  void stop() override;
  QList<Activity> drainEvents() override;

  void captureForegroundActivity();

private:
  WindowsActivityProvider() = default;

private:
  ActivityCollector _collector;
  BrowserInfoParser _browserInfo;
  win32::UniqueWinEventHook _foregroundHook;
  win32::UniqueWinEventHook _nameChangeHook;
};

} // namespace chronexa::activity
