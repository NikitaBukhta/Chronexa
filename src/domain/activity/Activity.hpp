#pragma once

#include <QDateTime>
#include <QString>

namespace chronexa::activity {

struct Activity {
  QString appName;
  QString title;
  QDateTime startedOn;
  QDateTime endedOn;
};

} // namespace chronexa::activity
