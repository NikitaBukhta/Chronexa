#pragma once

#include <QRegularExpression>
#include <QString>
#include <QStringList>

namespace chronexa::activity {

// The window condition shared by category and privacy rules. It holds when
// every part it sets holds:
//  - apps: any entry is a case-insensitive substring of the app name. A
//    substring rather than an exact name, because the recorded name is the
//    executable's description ("CLion (GUI launcher)"), which nobody types.
//  - titlePattern: a case-insensitive regular expression found in the title.
// An empty part is not checked. The regex is compiled once, here.
class WindowMatcher {
public:
  WindowMatcher() = default;
  WindowMatcher(const QStringList &apps, const QString &titlePattern);

  // Sets at least one part: without one it would match every window.
  bool hasCondition() const;
  // Whether the title pattern compiles; an empty one does.
  bool isTitleValid() const;
  // hasCondition() and isTitleValid().
  bool isUsable() const;

  // False for an unusable matcher.
  bool matches(const QString &appName, const QString &title) const;

private:
  QStringList _apps;
  QRegularExpression _title;
  bool _checksTitle = false;
  bool _titleValid = true;
};

// Trimmed, with the empty entries dropped.
QStringList normalizedApps(const QStringList &apps);

} // namespace chronexa::activity
