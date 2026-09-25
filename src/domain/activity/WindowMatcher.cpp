#include "WindowMatcher.hpp"

#include <algorithm>

namespace chronexa::activity {

QStringList normalizedApps(const QStringList &apps) {
  QStringList result;
  for (const QString &app : apps) {
    const QString trimmed = app.trimmed();
    if (!trimmed.isEmpty()) {
      result.append(trimmed);
    }
  }
  return result;
}

WindowMatcher::WindowMatcher(const QStringList &apps,
                             const QString &titlePattern)
    : _apps(normalizedApps(apps)),
      _checksTitle(!titlePattern.trimmed().isEmpty()) {
  if (_checksTitle) {
    _title = QRegularExpression(
        titlePattern, QRegularExpression::CaseInsensitiveOption |
                          QRegularExpression::UseUnicodePropertiesOption);
    _titleValid = _title.isValid();
    if (_titleValid) {
      _title.optimize();
    }
  }
}

bool WindowMatcher::hasCondition() const {
  return !_apps.isEmpty() || _checksTitle;
}

bool WindowMatcher::isTitleValid() const { return _titleValid; }

bool WindowMatcher::isUsable() const { return hasCondition() && _titleValid; }

bool WindowMatcher::matches(const QString &appName,
                            const QString &title) const {
  if (!isUsable()) {
    return false;
  }
  const bool appMatches =
      _apps.isEmpty() ||
      std::any_of(_apps.cbegin(), _apps.cend(), [&appName](const QString &app) {
        return appName.contains(app, Qt::CaseInsensitive);
      });
  if (!appMatches) {
    return false;
  }
  return !_checksTitle || _title.match(title).hasMatch();
}

} // namespace chronexa::activity
