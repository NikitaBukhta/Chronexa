#pragma once

#include <QDateTime>
#include <QString>

namespace chronexa::activity {

struct Activity {
  QString appName;
  QString title;
  QDateTime startedOn;
  QDateTime endedOn;

  bool isValid() const;
  qint64 durationSeconds() const;
};

} // namespace chronexa::activity
