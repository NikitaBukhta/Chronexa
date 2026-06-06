#include "WindowsActivityProvider.hpp"

#include "AppInfoParser.hpp"
#include "BrowserInfoParser.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QString>

#include <windows.h>

namespace {

Q_LOGGING_CATEGORY(lcProvider, "chronexa.activity.provider.windows")

void CALLBACK winEventProc(HWINEVENTHOOK /*hook*/, DWORD event, HWND hwnd, LONG idObject, LONG idChild,
                           DWORD /*idEventThread*/, DWORD /*dwmsEventTime*/) {
  auto *activityProvider = chronexa::activity::WindowsActivityProvider::instance();
  if (!activityProvider || idObject != OBJID_WINDOW || idChild != CHILDID_SELF) {
    return;
  }
  // Title changes are only interesting for the foreground window
  // (e.g. switching tabs in a browser).
  if (event == EVENT_OBJECT_NAMECHANGE && hwnd != GetForegroundWindow()) {
    return;
  }
  activityProvider->captureForegroundActivity();
}

} // namespace

namespace chronexa::activity {

WindowsActivityProvider::~WindowsActivityProvider() { WindowsActivityProvider::stop(); }

WindowsActivityProvider *WindowsActivityProvider::instance() {
  static auto *instance = new WindowsActivityProvider();
  return instance;
}

void WindowsActivityProvider::start() {
  if (_foregroundHook) {
    return;
  }

  _foregroundHook.reset(SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, nullptr, winEventProc, 0, 0,
                                        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS));
  _nameChangeHook.reset(SetWinEventHook(EVENT_OBJECT_NAMECHANGE, EVENT_OBJECT_NAMECHANGE, nullptr, winEventProc, 0, 0,
                                        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS));

  if (!_foregroundHook || !_nameChangeHook) {
    qCWarning(lcProvider) << "Failed to install WinEvent hooks, foreground =" << static_cast<bool>(_foregroundHook)
                          << ", name change =" << static_cast<bool>(_nameChangeHook);
  } else {
    qCInfo(lcProvider) << "WinEvent hooks installed";
  }

  captureForegroundActivity();
}

void WindowsActivityProvider::stop() {
  _foregroundHook.reset();
  _nameChangeHook.reset();
  _collector.reset();
  qCInfo(lcProvider) << "WinEvent hooks removed";
}

QList<Activity> WindowsActivityProvider::drainEvents() { return _collector.drainHandled(QDateTime::currentDateTime()); }

void WindowsActivityProvider::captureForegroundActivity() {
  const HWND hwnd = GetForegroundWindow();
  if (!hwnd) {
    return;
  }

  const AppInfoParser appInfo(hwnd);
  const QString appName = appInfo.appName();
  if (appName.isEmpty()) {
    qCDebug(lcProvider) << "Foreground window has no readable app info";
    return;
  }

  const QString appId = appInfo.appId();

  QString tabDomain;
  if (BrowserInfoParser::isSupportedBrowser(appId)) {
    tabDomain = _browserInfo.currentDomain(hwnd, appId);
  }

  _collector.notifyActivityChanged(appId, appName, appInfo.windowTitle(), tabDomain, QDateTime::currentDateTime());
}

} // namespace chronexa::activity
