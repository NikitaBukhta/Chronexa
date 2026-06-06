#include "AppInfoParser.hpp"

#include "Win32Raii.hpp"

#include <QFileInfo>
#include <QLoggingCategory>
#include <QSet>
#include <QVarLengthArray>

namespace {

Q_LOGGING_CATEGORY(lcAppInfo, "chronexa.activity.appinfo")

QString baseNameFromPath(const QString &path) { return QFileInfo(path).completeBaseName(); }

} // namespace

namespace chronexa::activity {

AppInfoParser::AppInfoParser(const HWND hwnd) : _hwnd(hwnd) {}

quint32 AppInfoParser::processId() const {
  DWORD pid = 0;
  GetWindowThreadProcessId(_hwnd, &pid);
  return pid;
}

QString AppInfoParser::executablePath() const {
  const DWORD pid = processId();
  if (pid == 0) {
    return {};
  }

  const win32::UniqueHandle process(OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
  if (!process) {
    qCWarning(lcAppInfo) << "Failed to open process" << pid << ", error =" << GetLastError();
    return {};
  }

  wchar_t path[MAX_PATH] = {};
  DWORD size = MAX_PATH;
  if (!QueryFullProcessImageNameW(process.get(), 0, path, &size)) {
    return {};
  }
  return QString::fromWCharArray(path, size);
}

QString AppInfoParser::appId() const {
  const QString path = executablePath();
  if (path.isEmpty()) {
    qCDebug(lcAppInfo) << "No executable path for window, falling back to app name";
    return appName();
  }
  return baseNameFromPath(path);
}

QString AppInfoParser::appName() const {
  // Standard Windows classes (dialogs, menus, ...) carry no meaningful
  // FileDescription, so prefer the ProductName for them.
  QString displayName = isStandardWindowsClass(className()) ? appMetadata(AppMetadata::ProductName)
                                                            : appMetadata(AppMetadata::FileDescription);

  if (displayName.isEmpty()) {
    displayName = baseNameFromPath(executablePath());
  }

  return displayName.isEmpty() ? windowTitle() : displayName;
}

QString AppInfoParser::windowTitle() const {
  const int length = GetWindowTextLengthW(_hwnd);
  if (length <= 0) {
    return {};
  }

  QString title(length, QChar(0));
  const int copied = GetWindowTextW(_hwnd, reinterpret_cast<LPWSTR>(title.data()), length + 1);
  title.truncate(copied);
  return title;
}

QString AppInfoParser::className() const {
  wchar_t buffer[256] = {};
  const int length = GetClassNameW(_hwnd, buffer, static_cast<int>(std::size(buffer)));
  return QString::fromWCharArray(buffer, length);
}

QString AppInfoParser::appMetadata(const AppMetadata metadata) const {
  const QString filePath = executablePath();
  if (filePath.isEmpty()) {
    return {};
  }

  const std::wstring nativePath = filePath.toStdWString();

  DWORD dummy = 0;
  const DWORD size = GetFileVersionInfoSizeW(nativePath.c_str(), &dummy);
  if (size == 0) {
    return {};
  }

  QVarLengthArray<BYTE, 4096> versionInfo(static_cast<int>(size));
  if (!GetFileVersionInfoW(nativePath.c_str(), 0, size, versionInfo.data())) {
    return {};
  }

  struct LangCodePage {
    WORD language;
    WORD codePage;
  };

  LangCodePage *translation = nullptr;
  UINT translationSize = 0;
  if (!VerQueryValueW(versionInfo.data(), L"\\VarFileInfo\\Translation", reinterpret_cast<void **>(&translation),
                      &translationSize) ||
      translationSize < sizeof(LangCodePage)) {
    return {};
  }

  const wchar_t *metadataName = (metadata == AppMetadata::ProductName) ? L"ProductName" : L"FileDescription";

  // Query the value for the first available language/code page.
  const QString subBlock = QStringLiteral("\\StringFileInfo\\%1%2\\%3")
                               .arg(translation[0].language, 4, 16, QChar('0'))
                               .arg(translation[0].codePage, 4, 16, QChar('0'))
                               .arg(QString::fromWCharArray(metadataName));

  void *buffer = nullptr;
  UINT bufferSize = 0;
  if (VerQueryValueW(versionInfo.data(), subBlock.toStdWString().c_str(), &buffer, &bufferSize) && bufferSize > 0) {
    return QString::fromWCharArray(static_cast<const wchar_t *>(buffer), static_cast<int>(bufferSize) - 1);
  }

  return {};
}

bool AppInfoParser::isStandardWindowsClass(const QString &className) {
  // https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-classes
  static const QSet<QString> kStandardClasses = {
      QStringLiteral("#32768"),    // Menu
      QStringLiteral("#32769"),    // Desktop
      QStringLiteral("#32770"),    // Dialog
      QStringLiteral("#32771"),    // Task switch
      QStringLiteral("#32772"),    // Icon titles
      QStringLiteral("Message"),   // Message-only window
      QStringLiteral("ComboLBox"), // Listbox in combo box
      QStringLiteral("DDEMLEvent") // DDEML events
  };

  return kStandardClasses.contains(className);
}

} // namespace chronexa::activity
