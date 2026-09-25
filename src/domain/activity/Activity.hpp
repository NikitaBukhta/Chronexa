#pragma once

#include <QDateTime>
#include <QString>

namespace chronexa::activity {

struct Activity {
  QString appName;
  QString title;
  QDateTime startedOn;
  QDateTime endedOn;

  [[nodiscard]] bool isValid() const;
  [[nodiscard]] qint64 durationSeconds() const;
};

struct WindowRef {
  QString appName;
  QString title;

  bool operator==(const WindowRef &other) const {
    return appName == other.appName && title == other.title;
  }
};

} // namespace chronexa::activity
