#pragma once

#include "WindowMatcher.hpp"

#include <QList>
#include <QString>
#include <QStringList>

namespace chronexa::activity {

// What happens to a window before anything about it is stored. Declared from
// least to most strict; PrivacyRules compares them.
enum class Privacy {
  // Recorded as it is.
  Record,
  // Recorded under its application, with the title left empty: the time
  // counts, the chat partner or message text does not reach the disk.
  HideTitle,
  // Not recorded at all, as if no window were in the foreground.
  Exclude,
};

// "KeePass -> exclude", "Telegram -> hide title". The window condition is a
// WindowMatcher, as for categories; a rule without one would apply to every
// window and is invalid.
struct PrivacyRule {
  Privacy action = Privacy::HideTitle;
  QStringList apps;
  QString titlePattern;

  bool operator==(const PrivacyRule &other) const;
  bool operator!=(const PrivacyRule &other) const;

  // Usable: acts, sets at least one condition, and its regex compiles.
  bool isValid() const;
};

// Unlike categories, order does not matter: the strictest matching rule
// decides, so an exclusion can never be undone by a hide-title rule listed
// above it.
class PrivacyRules {
public:
  PrivacyRules() = default;
  explicit PrivacyRules(QList<PrivacyRule> rules);

  Privacy classify(const QString &appName, const QString &title) const;

  // Per rule, by its index in rules().
  bool isUsable(int index) const;
  bool hasValidTitle(int index) const;

  const QList<PrivacyRule> &rules() const;
  bool isEmpty() const;

private:
  bool isIndex(int index) const;

  QList<PrivacyRule> _rules;
  // Parallel to _rules, unusable rules included, so indexes line up.
  QList<WindowMatcher> _matchers;
};

// The action's stable key in settings and in QML: "exclude" or "hideTitle".
QString privacyKey(Privacy action);
// Record for an unknown key, which makes the rule unusable.
Privacy privacyFromKey(const QString &key);

QString serializePrivacyRules(const QList<PrivacyRule> &rules);
QList<PrivacyRule> parsePrivacyRules(const QString &json);

} // namespace chronexa::activity
