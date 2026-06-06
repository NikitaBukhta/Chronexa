#pragma once

#include <QString>

#include <windows.h>

namespace chronexa::activity {

class AppInfoParser {
public:
  explicit AppInfoParser(HWND hwnd);

  [[nodiscard]] quint32 processId() const;
  [[nodiscard]] QString executablePath() const;
  [[nodiscard]] QString appId() const;
  [[nodiscard]] QString appName() const;
  [[nodiscard]] QString windowTitle() const;

private:
  enum class AppMetadata { ProductName, FileDescription };

  [[nodiscard]] QString className() const;
  [[nodiscard]] QString appMetadata(AppMetadata metadata) const;
  static bool isStandardWindowsClass(const QString &className);

private:
  HWND _hwnd = nullptr;
};

} // namespace chronexa::activity
