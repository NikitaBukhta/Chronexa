#include "AppEnvironment.hpp"

#include "FileLogger.hpp"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSettings>
#include <QStandardPaths>

#include <memory>

namespace {

Q_LOGGING_CATEGORY(lcAppEnv, "chronexa.core.AppEnvironment")

std::unique_ptr<chronexa::core::FileLogger> g_logger;

QString g_profileDir;

} // namespace

namespace chronexa::core {

void AppEnvironment::useProfileDir(const QString &dir) {
  g_profileDir = QDir(dir).absolutePath();
  QDir().mkpath(g_profileDir);

  // Covers both AppSettings and the QML Settings objects (Theme), which all
  // construct QSettings with the default format.
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, g_profileDir);
}

QString AppEnvironment::ensureDataDir() {
  if (!g_profileDir.isEmpty()) {
    return g_profileDir;
  }

  QString path =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(path);
  dir.cdUp();
  path = dir.absoluteFilePath("Chronexa");

  QDir().mkpath(path);
  return path;
}

QString AppEnvironment::dataPath() {
  static const QString path = ensureDataDir();
  return path;
}

QString AppEnvironment::databasePath() { return dataPath() + "/chronexa.db"; }

QString AppEnvironment::logFilePath() {
  const QString timestamp =
      QDateTime::currentDateTime().toString("dd.MM.yyyy-hh.mm.ss");
  return dataPath() + "/log_" + timestamp + ".log";
}

void AppEnvironment::installFileLogger() {
  cleanupOldLogs();
  g_logger = std::make_unique<FileLogger>(logFilePath());
}

void AppEnvironment::shutdownFileLogger() { g_logger.reset(); }

void AppEnvironment::cleanupOldLogs(int keepDays) {
  const QDir dir(dataPath());
  const QDateTime cutoff = QDateTime::currentDateTime().addDays(-keepDays);

  const auto entries =
      dir.entryInfoList({"log_*.log", "log_*.log.*"}, QDir::Files, QDir::Time);
  for (const QFileInfo &info : entries) {
    if (info.lastModified() < cutoff) {
      if (QFile::remove(info.absoluteFilePath()))
        qCInfo(lcAppEnv) << "Removed old log:" << info.fileName();
    }
  }
}

} // namespace chronexa::core
