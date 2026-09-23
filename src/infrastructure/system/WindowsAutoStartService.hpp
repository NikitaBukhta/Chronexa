#pragma once

#include "IAutoStartService.hpp"

#include <QString>

namespace chronexa::system {

class WindowsAutoStartService : public IAutoStartService {
public:
  explicit WindowsAutoStartService(
      QString valueName = QStringLiteral("Chronexa"));

  bool isSupported() const override;
  bool isEnabled() const override;
  bool setEnabled(bool enabled) override;

private:
  QString launchCommand() const;

  QString _valueName;
};

} // namespace chronexa::system
