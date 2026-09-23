#pragma once

namespace chronexa::system {

class IAutoStartService {
public:
  virtual ~IAutoStartService() = default;

  virtual bool isSupported() const = 0;
  virtual bool isEnabled() const = 0;
  virtual bool setEnabled(bool enabled) = 0;
};

} // namespace chronexa::system
