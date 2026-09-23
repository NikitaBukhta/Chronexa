#pragma once

#include "IForegroundProbe.hpp"

#include <QHash>
#include <QString>

namespace chronexa::activity {

class WindowsForegroundProbe : public IForegroundProbe {
public:
  ForegroundSample sample() override;

private:
  QString appNameForWindow(void *windowHandle);

private:
  QHash<QString, QString> _appNameCache;
};

} // namespace chronexa::activity
