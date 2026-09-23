#include "WindowsAutoStartService.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QLoggingCategory>
#include <QSettings>

namespace {

Q_LOGGING_CATEGORY(lcAutoStart, "chronexa.system.autostart")

constexpr auto kRunKey =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

} // namespace

namespace chronexa::system {

WindowsAutoStartService::WindowsAutoStartService(QString valueName)
    : _valueName(std::move(valueName)) {}

bool WindowsAutoStartService::isSupported() const { return true; }

QString WindowsAutoStartService::launchCommand() const {
  const QString path =
      QDir::toNativeSeparators(QCoreApplication::applicationFilePath());
  return QStringLiteral("\"%1\"").arg(path);
}

bool WindowsAutoStartService::isEnabled() const {
  const QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);
  const QString registered = run.value(_valueName).toString();
  if (registered.isEmpty()) {
    return false;
  }

  return registered.compare(launchCommand(), Qt::CaseInsensitive) == 0;
}

bool WindowsAutoStartService::setEnabled(bool enabled) {
  QSettings run(QLatin1String(kRunKey), QSettings::NativeFormat);

  if (enabled) {
    run.setValue(_valueName, launchCommand());
  } else {
    run.remove(_valueName);
  }
  run.sync();

  if (run.status() != QSettings::NoError) {
    qCWarning(lcAutoStart) << "Could not update the Run key, status ="
                           << run.status();
    return false;
  }

  qCInfo(lcAutoStart) << "Launch at sign-in"
                      << (enabled ? "enabled" : "disabled");
  return true;
}

} // namespace chronexa::system
