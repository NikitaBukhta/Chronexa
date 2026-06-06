#include "FileLogger.hpp"

#include <QDateTime>
#include <QLoggingCategory>
#include <QThread>

#include <cassert>
#include <cstring>
#include <iostream>

#ifdef Q_OS_ANDROID
#include <android/log.h>
#endif

namespace chronexa::core {

namespace {

constexpr qint64 kMaxLogSizeBytes = 10 * 1024 * 1024;
constexpr int kMaxRotatedFiles = 5;

QString parseShortFunctionName(const char *pretty) {
  const char *begin = pretty;
  const char *end = pretty + std::strlen(pretty);

  for (const char *p = end; p > begin; --p) {
    if (*(p - 1) == '(') {
      end = p - 1;
      break;
    }
  }
  while (end > begin && static_cast<unsigned char>(*(end - 1)) <= ' ') {
    --end;
  }

  const char *start = end;
  while (start > begin && *(start - 1) != ' ') {
    --start;
  }

  const char *lastSep = nullptr;
  const char *prevSep = nullptr;
  for (const char *p = end - 1; p > start; --p) {
    if (p[-1] == ':' && p[0] == ':') {
      if (!lastSep) {
        lastSep = p - 1;
      } else {
        prevSep = p - 1;
        break;
      }
    }
  }
  if (prevSep) {
    start = prevSep + 2;
  }

  return QString::fromLatin1(start, static_cast<qsizetype>(end - start));
}

} // namespace

FileLogger *FileLogger::s_instance = nullptr;

FileLogger::FileLogger(QString filePath) : _file(filePath), _filePath(std::move(filePath)) {
  assert(s_instance == nullptr && "FileLogger is a singleton");

  if (!_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    return;
  }

  s_instance = this;
  _writerThread = std::thread([this] { writerLoop(); });

  qInstallMessageHandler(&FileLogger::messageHandler);
}

FileLogger::~FileLogger() {
  qInstallMessageHandler(nullptr);

  _stopRequested.store(true, std::memory_order_release);
  _queueCv.notify_all();
  if (_writerThread.joinable()) {
    _writerThread.join();
  }

  if (_file.isOpen()) {
    _file.flush();
    _file.close();
  }

  s_instance = nullptr;
}

void FileLogger::enqueue(QString line) {
  {
    const std::lock_guard lock(_queueMutex);
    _queue.push_back(std::move(line));
  }
  _queueCv.notify_one();
}

void FileLogger::writeSync(const QString &line) {
  if (_file.isOpen()) {
    _file.write(line.toUtf8());
    _file.flush();
  }
}

QString FileLogger::shortFunctionName(const char *pretty) {
  if (!pretty) {
    return QStringLiteral("?");
  }
  {
    const std::lock_guard lock(_funcNameCacheMutex);
    const auto it = _funcNameCache.find(pretty);
    if (it != _funcNameCache.end()) {
      return it->second;
    }
  }
  QString computed = parseShortFunctionName(pretty);
  {
    const std::lock_guard lock(_funcNameCacheMutex);
    return _funcNameCache.emplace(pretty, std::move(computed)).first->second;
  }
}

void FileLogger::rotate() {
  _file.flush();
  _file.close();

  QFile::remove(_filePath + QStringLiteral(".") + QString::number(kMaxRotatedFiles));
  for (int i = kMaxRotatedFiles - 1; i >= 1; --i) {
    const QString src = _filePath + QStringLiteral(".") + QString::number(i);
    const QString dst = _filePath + QStringLiteral(".") + QString::number(i + 1);
    if (QFile::exists(src)) {
      QFile::rename(src, dst);
    }
  }
  QFile::rename(_filePath, _filePath + QStringLiteral(".1"));

  _file.setFileName(_filePath);
  if (!_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
    std::cerr << "FileLogger: failed to reopen log after rotation: " << _filePath.toLocal8Bit().constData() << ": "
              << _file.errorString().toLocal8Bit().constData() << '\n';
  }
}

void FileLogger::writeLine(const QString &line) {
  if (!_file.isOpen()) {
    return;
  }
  _file.write(line.toUtf8());

  if (_file.pos() >= kMaxLogSizeBytes) {
    rotate();
  }
}

void FileLogger::writerLoop() {
  std::deque<QString> local;
  while (true) {
    {
      std::unique_lock lock(_queueMutex);
      _queueCv.wait(lock, [this] { return !_queue.empty() || _stopRequested.load(std::memory_order_acquire); });
      local.swap(_queue);
    }

    for (const QString &line : local) {
      writeLine(line);
    }
    local.clear();

    if (_file.isOpen()) {
      _file.flush();
    }

    if (_stopRequested.load(std::memory_order_acquire)) {
      std::unique_lock lock(_queueMutex);
      if (_queue.empty()) {
        return;
      }
    }
  }
}

void FileLogger::messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
  FileLogger *self = s_instance;
  if (!self) {
    return;
  }

  const char *level = nullptr;
  switch (type) {
  case QtDebugMsg:
    level = "DEBUG";
    break;
  case QtInfoMsg:
    level = "INFO ";
    break;
  case QtWarningMsg:
    level = "WARN ";
    break;
  case QtCriticalMsg:
    level = "CRIT ";
    break;
  case QtFatalMsg:
    level = "FATAL";
    break;
  }

  const QString timestamp = QDateTime::currentDateTime().toString(Qt::ISODateWithMs);
  const QString category = context.category ? context.category : "default";
  const QString function = self->shortFunctionName(context.function);
  const QString thread = QString::asprintf("%p", static_cast<void *>(QThread::currentThread()));

  QString line = QStringLiteral("%1 [%2] [%3] %4 %5: %6\n").arg(timestamp, level, thread, category, function, msg);

#ifdef Q_OS_ANDROID
  android_LogPriority prio = ANDROID_LOG_INFO;
  switch (type) {
  case QtDebugMsg:
    prio = ANDROID_LOG_DEBUG;
    break;
  case QtInfoMsg:
    prio = ANDROID_LOG_INFO;
    break;
  case QtWarningMsg:
    prio = ANDROID_LOG_WARN;
    break;
  case QtCriticalMsg:
    prio = ANDROID_LOG_ERROR;
    break;
  case QtFatalMsg:
    prio = ANDROID_LOG_FATAL;
    break;
  }
  const QString body = QStringLiteral("[%1] %2").arg(category, msg);
  __android_log_write(prio, "BeeLibrary", body.toUtf8().constData());
#elif !defined(QT_NO_DEBUG)
  std::cerr << line.toLocal8Bit().constData();
#endif

  if (type == QtFatalMsg) {
    // Bypass the queue so the line survives an imminent abort().
    self->writeSync(line);
    return;
  }

  self->enqueue(std::move(line));
}

} // namespace chronexa::core
