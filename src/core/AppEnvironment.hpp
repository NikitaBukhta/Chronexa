#pragma once

#include <QString>

namespace chronexa::core {

class AppEnvironment {
public:
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
