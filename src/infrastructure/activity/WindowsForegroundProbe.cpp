#include "WindowsForegroundProbe.hpp"

#include <QFileInfo>

#include <windows.h>

#include <psapi.h>

namespace {

constexpr qint64 kIdleThresholdSeconds = 120;

qint64 idleSeconds() {
  LASTINPUTINFO info{};
  info.cbSize = sizeof(info);
  if (!GetLastInputInfo(&info)) {
    return 0;
  }
  // dwTime is the 32-bit tick count, so it must be compared against the
  // 32-bit clock: the unsigned wrap-around then cancels out by itself.
  return static_cast<qint64>(GetTickCount() - info.dwTime) / 1000;
}

QString executablePath(HWND windowHandle) {
  DWORD processId = 0;
  GetWindowThreadProcessId(windowHandle, &processId);
  if (processId == 0) {
    return {};
  }

  const HANDLE process =
      OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
  if (process == nullptr) {
    return {};
  }

  wchar_t buffer[MAX_PATH] = {};
  DWORD size = MAX_PATH;
  QString path;
  if (QueryFullProcessImageNameW(process, 0, buffer, &size)) {
    path = QString::fromWCharArray(buffer, static_cast<int>(size));
  }
  CloseHandle(process);
  return path;
}

QString fileDescription(const QString &executablePath) {
  const std::wstring path = executablePath.toStdWString();

  DWORD handle = 0;
  const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
  if (size == 0) {
    return {};
  }

  QByteArray block(static_cast<int>(size), Qt::Uninitialized);
  if (!GetFileVersionInfoW(path.c_str(), handle, size, block.data())) {
    return {};
  }

  struct Translation {
    WORD language;
    WORD codePage;
  };
  Translation *translations = nullptr;
  UINT translationBytes = 0;
  if (!VerQueryValueW(block.constData(), L"\\VarFileInfo\\Translation",
                      reinterpret_cast<LPVOID *>(&translations),
                      &translationBytes) ||
      translationBytes < sizeof(Translation)) {
    return {};
  }

  const QString key =
      QStringLiteral("\\StringFileInfo\\%1%2\\FileDescription")
          .arg(translations[0].language, 4, 16, QLatin1Char('0'))
          .arg(translations[0].codePage, 4, 16, QLatin1Char('0'));

  wchar_t *value = nullptr;
  UINT valueLength = 0;
  if (!VerQueryValueW(block.constData(), key.toStdWString().c_str(),
                      reinterpret_cast<LPVOID *>(&value), &valueLength) ||
      valueLength == 0) {
    return {};
  }

  // The reported length counts the terminator; passing it through would
  // leave a NUL inside the name.
  return QString::fromWCharArray(value).trimmed();
}

QString foregroundWindowTitle(HWND windowHandle) {
  const int length = GetWindowTextLengthW(windowHandle);
  if (length <= 0) {
    return {};
  }

  std::wstring buffer(static_cast<size_t>(length) + 1, L'\0');
  const int written = GetWindowTextW(windowHandle, buffer.data(),
                                     static_cast<int>(buffer.size()));
  return QString::fromWCharArray(buffer.data(), written);
}

} // namespace

namespace chronexa::activity {

ForegroundSample WindowsForegroundProbe::sample() {
  ForegroundSample result;
  result.idle = idleSeconds() >= kIdleThresholdSeconds;
  if (result.idle) {
    return result;
  }

  const HWND windowHandle = GetForegroundWindow();
  if (windowHandle == nullptr) {
    return result;
  }

  result.appName = appNameForWindow(windowHandle);
  if (!result.appName.isEmpty()) {
    result.title = foregroundWindowTitle(windowHandle);
  }
  return result;
}

QString WindowsForegroundProbe::appNameForWindow(void *windowHandle) {
  const QString path = executablePath(static_cast<HWND>(windowHandle));
  if (path.isEmpty()) {
    return {};
  }

  const auto cached = _appNameCache.constFind(path);
  if (cached != _appNameCache.constEnd()) {
    return *cached;
  }

  QString name = fileDescription(path);
  if (name.isEmpty()) {
    name = QFileInfo(path).completeBaseName();
  }
  _appNameCache.insert(path, name);
  return name;
}

} // namespace chronexa::activity
