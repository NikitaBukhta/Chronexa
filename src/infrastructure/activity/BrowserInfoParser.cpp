#include "BrowserInfoParser.hpp"

#include "Win32Raii.hpp"

#include <QLoggingCategory>
#include <QRegularExpression>
#include <QSet>

#include <objbase.h>

namespace {

Q_LOGGING_CATEGORY(lcBrowser, "chronexa.activity.browser")

const QSet<QString> kSupportedBrowsers = {
    QStringLiteral("chrome"),
    QStringLiteral("msedge"),
    QStringLiteral("firefox"),
    QStringLiteral("opera"),
};

bool looksLikeDomain(const QString &host) {
  static const QRegularExpression kDomainRegex(QStringLiteral(
      R"(^[A-Za-z0-9](?:[A-Za-z0-9\-]{0,61}[A-Za-z0-9])?(?:\.[A-Za-z0-9](?:[A-Za-z0-9\-]{0,61}[A-Za-z0-9])?)+$)"));

  if (host.isEmpty() || host.size() > 253 || host.contains(QChar(' '))) {
    return false;
  }
  if (host == QLatin1String("localhost") || host == QLatin1String("127.0.0.1") || host == QLatin1String("[::1]")) {
    return true;
  }
  return kDomainRegex.match(host).hasMatch();
}

} // namespace

namespace chronexa::activity {

bool BrowserInfoParser::isSupportedBrowser(const QString &appId) { return kSupportedBrowsers.contains(appId); }

QString BrowserInfoParser::currentDomain(const HWND hwnd, const QString &appId) {
  if (!ensureAutomation()) {
    return {};
  }

  // Fast path: cached address-bar element -- a single property fetch.
  if (const auto it = _addressBarCache.constFind(hwnd); it != _addressBarCache.constEnd()) {
    const QString value = readValue(it.value());
    if (!value.isEmpty()) {
      return domainFromAddress(value);
    }
    // Element became stale (window/control re-created) -- re-resolve.
    _addressBarCache.remove(hwnd);
  }

  ElementPtr addressBar = findAddressBar(hwnd, appId);
  if (!addressBar) {
    qCDebug(lcBrowser) << "Address bar not found for" << appId;
    return {};
  }

  _addressBarCache.insert(hwnd, addressBar);
  return domainFromAddress(readValue(addressBar));
}

bool BrowserInfoParser::ensureAutomation() {
  if (_automation) {
    return true;
  }

  // Qt already initializes COM (STA) on the GUI thread; S_FALSE and
  // RPC_E_CHANGED_MODE just mean it is usable as-is.
  const HRESULT initResult = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  if (FAILED(initResult) && initResult != RPC_E_CHANGED_MODE) {
    qCWarning(lcBrowser) << "CoInitializeEx failed:" << initResult;
    return false;
  }

  const HRESULT result =
      CoCreateInstance(CLSID_CUIAutomation, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&_automation));
  if (FAILED(result)) {
    qCWarning(lcBrowser) << "Failed to create IUIAutomation:" << result;
    return false;
  }
  return true;
}

BrowserInfoParser::ElementPtr BrowserInfoParser::findAddressBar(const HWND hwnd, const QString &appId) {
  ElementPtr root;
  if (FAILED(_automation->ElementFromHandle(hwnd, &root)) || !root) {
    return {};
  }

  return (appId == QLatin1String("firefox")) ? findFirefoxAddressBar(root) : findChromiumAddressBar(root);
}

// Firefox: the url input is "urlbar-input" inside the "nav-bar" toolbar.
// Searching the toolbar first keeps the descendant scan small.
BrowserInfoParser::ElementPtr BrowserInfoParser::findFirefoxAddressBar(const ElementPtr &root) {
  Microsoft::WRL::ComPtr<IUIAutomationCondition> navBarCondition;
  const auto navBarId = win32::ScopedVariant::fromBstr(L"nav-bar");
  _automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, navBarId.get(), &navBarCondition);

  ElementPtr navBar;
  if (!navBarCondition || FAILED(root->FindFirst(TreeScope_Children, navBarCondition.Get(), &navBar)) || !navBar) {
    return {};
  }

  Microsoft::WRL::ComPtr<IUIAutomationCondition> urlBarCondition;
  const auto urlBarId = win32::ScopedVariant::fromBstr(L"urlbar-input");
  _automation->CreatePropertyCondition(UIA_AutomationIdPropertyId, urlBarId.get(), &urlBarCondition);

  ElementPtr addressBar;
  if (!urlBarCondition || FAILED(navBar->FindFirst(TreeScope_Descendants, urlBarCondition.Get(), &addressBar))) {
    return {};
  }
  return addressBar;
}

// Chromium-based browsers (Chrome, Edge, Opera): the address bar is the
// first Edit control in the window.
BrowserInfoParser::ElementPtr BrowserInfoParser::findChromiumAddressBar(const ElementPtr &root) {
  Microsoft::WRL::ComPtr<IUIAutomationCondition> condition;
  const auto controlType = win32::ScopedVariant::fromInt(UIA_EditControlTypeId);
  _automation->CreatePropertyCondition(UIA_ControlTypePropertyId, controlType.get(), &condition);

  ElementPtr addressBar;
  if (!condition || FAILED(root->FindFirst(TreeScope_Descendants, condition.Get(), &addressBar))) {
    return {};
  }
  return addressBar;
}

QString BrowserInfoParser::readValue(const ElementPtr &element) {
  win32::ScopedVariant value;
  if (FAILED(element->GetCurrentPropertyValue(UIA_ValueValuePropertyId, value.receive()))) {
    return {};
  }

  const VARIANT &raw = value.get();
  if (raw.vt != VT_BSTR || !raw.bstrVal) {
    return {};
  }
  return QString::fromWCharArray(raw.bstrVal, static_cast<int>(SysStringLen(raw.bstrVal)));
}

// "https://www.github.com/foo/bar" -> "github.com". Returns an empty
// string when the address bar does not hold a valid address (e.g. a typed
// search query).
QString BrowserInfoParser::domainFromAddress(const QString &address) {
  QString host = address.trimmed();

  if (const qsizetype schemePos = host.indexOf(QLatin1String("://")); schemePos != -1) {
    host = host.mid(schemePos + 3);
  }
  if (host.startsWith(QLatin1String("www."))) {
    host = host.mid(4);
  }
  if (const qsizetype slashPos = host.indexOf(QChar('/')); slashPos != -1) {
    host.truncate(slashPos);
  }

  return looksLikeDomain(host) ? host : QString();
}

} // namespace chronexa::activity
