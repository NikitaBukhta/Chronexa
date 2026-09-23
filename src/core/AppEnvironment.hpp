#pragma once

#include <QString>

namespace chronexa::core {

class AppEnvironment {
public:
  // Redirects history, logs and settings into one directory, away from the
  // user's profile. Must run before anything reads a path or a setting; the
  // end-to-end tests use it so they never touch real data.
  static void useProfileDir(const QString &dir);

  static QString dataPath();
  static QString databasePath();
  static QString logFilePath();

  static void installFileLogger();
  static void shutdownFileLogger();

private:
  static QString ensureDataDir();
  static void cleanupOldLogs(int keepDays = 7);
};

} // namespace chronexa::core
