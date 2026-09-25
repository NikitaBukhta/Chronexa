#pragma once

#include "ActivityStats.hpp"
#include "WindowMatcher.hpp"

#include <QList>
#include <QString>
#include <QStringList>

namespace chronexa::activity {

// "CLion, Visual Studio Code -> Work", "YouTube in a Chrome title ->
// Distractions". A rule matches as a WindowMatcher over its apps and title
// pattern; a rule with neither would claim every window and is invalid.
struct CategoryRule {
  QString category;
  QStringList apps;
  QString titlePattern;

  bool operator==(const CategoryRule &other) const;
  bool operator!=(const CategoryRule &other) const;

  // Usable: has a name, sets at least one condition, and its regex compiles.
  bool isValid() const;
};

// An ordered rule set: the first valid rule that matches decides, so narrow
// rules ("YouTube in Chrome") belong before broad ones ("Chrome"). Each regex
// is compiled once, here; everything else reads the result.
class CategoryRules {
public:
  CategoryRules() = default;
  explicit CategoryRules(QList<CategoryRule> rules);

  // The category name, or an empty string when no rule matches.
  QString categorize(const QString &appName, const QString &title) const;

  // Distinct names of the usable rules, in rule order.
  const QStringList &categoryNames() const;

  // Per rule, by its index in rules(): usable at all, whether its title
  // pattern compiles (an empty one does), and the index of its category in
  // categoryNames() (-1 when unusable).
  bool isUsable(int index) const;
  bool hasValidTitle(int index) const;
  int categoryIndex(int index) const;

  const QList<CategoryRule> &rules() const;
  bool isEmpty() const;

private:
  struct Compiled {
    QString category;
    WindowMatcher matcher;
    int categoryIndex = -1;
  };

  bool isIndex(int index) const;

  QList<CategoryRule> _rules;
  // Parallel to _rules, unusable rules included, so indexes line up.
  QList<Compiled> _compiled;
  QStringList _categoryNames;
};

struct CategoryTotal {
  // Empty for the time no rule claimed.
  QString category;
  qint64 seconds = 0;
  int sessionCount = 0;
};

// Largest first, ties by name; the uncategorized rest is always last.
QList<CategoryTotal> categoryTotals(const QList<TitleTotal> &titles,
                                    const CategoryRules &rules);

QString serializeCategoryRules(const QList<CategoryRule> &rules);
QList<CategoryRule> parseCategoryRules(const QString &json);

} // namespace chronexa::activity
