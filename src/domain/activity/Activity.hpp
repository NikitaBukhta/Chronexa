#pragma once

#include <QDateTime>
#include <QString>

namespace chronexa::activity {

struct Activity {
  QString appId;
  QString appName;
  QString title;
  // Domain of the active browser tab; empty for non-browser activity.
  QString tabDomain;
  QDateTime startedOn;
  QDateTime endedOn;
};

} // namespace chronexa::activity
