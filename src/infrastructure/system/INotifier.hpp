#pragma once

#include <QString>

namespace chronexa::system {

// Shows a short message outside the main window, e.g. a tray balloon.
class INotifier {
public:
  virtual ~INotifier() = default;

  virtual bool isSupported() const = 0;
  virtual void notify(const QString &title, const QString &message) = 0;
};

} // namespace chronexa::system
