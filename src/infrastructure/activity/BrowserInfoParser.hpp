#pragma once

#include <QHash>
#include <QString>

#include <uiautomation.h>
#include <windows.h>
#include <wrl/client.h>

namespace chronexa::activity {

// Resolves the domain of the active tab of a supported browser window via
// UI Automation.
//
// The expensive part of UIA is the descendant search for the address-bar
// element, so it is performed once per browser window and the element is
// cached; subsequent reads are a single property fetch. A stale cached
// element (window closed, browser re-created the control) is dropped and
// re-resolved on the next request.
class BrowserInfoParser {
public:
  static bool isSupportedBrowser(const QString &appId);

  // Domain of the active tab (e.g. "github.com"), or an empty string when
  // the address bar cannot be read or does not hold a valid address
  // (e.g. the user is typing a search query).
  QString currentDomain(HWND hwnd, const QString &appId);

private:
  using AutomationPtr = Microsoft::WRL::ComPtr<IUIAutomation>;
  using ElementPtr = Microsoft::WRL::ComPtr<IUIAutomationElement>;

  bool ensureAutomation();
  ElementPtr findAddressBar(HWND hwnd, const QString &appId);
  ElementPtr findFirefoxAddressBar(const ElementPtr &root);
  ElementPtr findChromiumAddressBar(const ElementPtr &root);

  static QString readValue(const ElementPtr &element);
  static QString domainFromAddress(const QString &address);

  AutomationPtr _automation;
  QHash<HWND, ElementPtr> _addressBarCache;
};

} // namespace chronexa::activity
