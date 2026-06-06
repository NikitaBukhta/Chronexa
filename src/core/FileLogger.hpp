#pragma once

#include <QFile>
#include <QString>
#include <QtGlobal>

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <thread>
#include <unordered_map>

class QMessageLogContext;

namespace chronexa::core {

class FileLogger {
public:
  explicit FileLogger(QString filePath);
  ~FileLogger();

  FileLogger(const FileLogger &) = delete;
  FileLogger &operator=(const FileLogger &) = delete;

  static FileLogger *instance() { return s_instance; }
  static void messageHandler(QtMsgType type, const QMessageLogContext &context,
                             const QString &msg);

private:
  void enqueue(QString line);
  void writeSync(const QString &line);
  QString shortFunctionName(const char *pretty);

  void writerLoop();
  void writeLine(const QString &line);
  void rotate();

  QFile _file;
  QString _filePath;

  std::deque<QString> _queue;
  std::mutex _queueMutex;
  std::condition_variable _queueCv;
  std::atomic<bool> _stopRequested{false};
  std::thread _writerThread;

  std::unordered_map<const char *, QString> _funcNameCache;
  std::mutex _funcNameCacheMutex;

  static FileLogger *s_instance;
};

} // namespace chronexa::core
