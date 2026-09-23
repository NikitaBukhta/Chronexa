#include "WindowsActivityProvider.hpp"

#include <QDateTime>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QMutexLocker>
#include <QTimer>

#include <windows.h>

#include <psapi.h>

namespace {

Q_LOGGING_CATEGORY(lcProvider, "chronexa.activity.provider")

constexpr int kPollIntervalMs = 1000;

constexpr qint64 kIdleThresholdSeconds = 120;

constexpr qint64 kMinSessionSeconds = 2;

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

WindowsActivityProvider::WindowsActivityProvider() = default;

WindowsActivityProvider::~WindowsActivityProvider() { stop(); }

void WindowsActivityProvider::start() {
  if (_running) {
    return;
  }

  if (!_timer) {
    _timer = std::make_unique<QTimer>();
    _timer->setTimerType(Qt::CoarseTimer);
    _timer->setInterval(kPollIntervalMs);
    QObject::connect(_timer.get(), &QTimer::timeout, _timer.get(),
                     [this]() { poll(); });
  }

  _running = true;
  _timer->start();
  qCInfo(lcProvider) << "Foreground sampling started every" << kPollIntervalMs
                     << "ms";
}

void WindowsActivityProvider::stop() {
  if (!_running) {
    return;
  }
  _running = false;

  if (_timer) {
    _timer->stop();
  }

  QMutexLocker locker(&_mutex);
  closeCurrentLocked(QDateTime::currentDateTime());
  qCInfo(lcProvider) << "Foreground sampling stopped";
}

bool WindowsActivityProvider::isRunning() const { return _running; }

bool WindowsActivityProvider::isIdle() const {
  QMutexLocker locker(&_mutex);
  return _idle;
}

void WindowsActivityProvider::poll() {
  const QDateTime now = QDateTime::currentDateTime();
  const bool idle = idleSeconds() >= kIdleThresholdSeconds;

  QMutexLocker locker(&_mutex);

  if (idle) {
    if (!_idle) {
      closeCurrentLocked(now);
      _idle = true;
      qCDebug(lcProvider) << "User went idle";
    }
    return;
  }
  _idle = false;

  const HWND windowHandle = GetForegroundWindow();
  if (windowHandle == nullptr) {
    closeCurrentLocked(now);
    return;
  }

  locker.unlock();
  const QString appName = appNameForWindow(windowHandle);
  const QString title = foregroundWindowTitle(windowHandle);
  locker.relock();

  if (appName.isEmpty()) {
    closeCurrentLocked(now);
    return;
  }

  // Keyed on the application and the session key, not on (app, title): a
  // media player, a terminal printing progress or a browser tab with a
  // countdown rewrites its title on every poll, and splitting the session there
  // produced nothing but 1-second fragments that closeCurrentLocked discarded
  // below the minimum -- so those apps recorded no time at all. The key is the
  // title's category, so switching from a work tab to YouTube does split,
  // while a ticking title stays one session. Otherwise the title is
  // descriptive, and follows the window it belongs to.
  const QString key = _sessionKey ? _sessionKey(appName, title) : QString();
  if (_hasCurrent && _current.appName == appName && _currentKey == key) {
    _current.title = title;
    _current.endedOn = now;
    return;
  }

  closeCurrentLocked(now);
  openCurrentLocked(appName, title, key, now);
}

void WindowsActivityProvider::closeCurrentLocked(const QDateTime &at) {
  if (!_hasCurrent) {
    return;
  }

  _current.endedOn = at.isValid() ? at : _current.endedOn;
  if (_current.durationSeconds() >= kMinSessionSeconds) {
    _buffer.append(_current);
  }

  _current = Activity();
  _currentKey.clear();
  _currentSince = QDateTime();
  _hasCurrent = false;
}

void WindowsActivityProvider::openCurrentLocked(const QString &appName,
                                                const QString &title,
                                                const QString &key,
                                                const QDateTime &at) {
  _currentKey = key;
  _current.appName = appName;
  _current.title = title;
  _current.startedOn = at;
  _current.endedOn = at;
  _currentSince = at;
  _hasCurrent = true;
}

void WindowsActivityProvider::setSessionKey(SessionKey key) {
  QMutexLocker locker(&_mutex);
  _sessionKey = std::move(key);
}

QList<Activity> WindowsActivityProvider::drainEvents() {
  QMutexLocker locker(&_mutex);

  // Split the open session: hand over the part that already happened and let
  // the rest keep running, so draining neither loses nor double counts it.
  if (_hasCurrent && _current.durationSeconds() >= kMinSessionSeconds) {
    _buffer.append(_current);
    _current.startedOn = _current.endedOn;
  }

  QList<Activity> drained;
  drained.swap(_buffer);
  return drained;
}

std::optional<Activity> WindowsActivityProvider::currentSession() const {
  QMutexLocker locker(&_mutex);
  if (!_hasCurrent || _idle) {
    return std::nullopt;
  }

  Activity session = _current;
  session.startedOn = _currentSince;
  return session;
}

QString WindowsActivityProvider::appNameForWindow(void *windowHandle) {
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
