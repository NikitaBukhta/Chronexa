#pragma once

#include "ActivityStats.hpp"
#include "WindowMatcher.hpp"

#include <QList>
#include <QString>
#include <QStringList>

namespace chronexa::activity {

struct CategoryRule {
  QString category;
  QStringList apps;
  QString titlePattern;

  bool operator==(const CategoryRule &other) const;
  bool operator!=(const CategoryRule &other) const;

  [[nodiscard]] bool isValid() const;
};

class CategoryRules {
public:
  CategoryRules() = default;
  explicit CategoryRules(QList<CategoryRule> rules);

  [[nodiscard]] QString categorize(const QString &appName,
                                   const QString &title) const;
  [[nodiscard]] const QStringList &categoryNames() const;
  [[nodiscard]] bool isUsable(int index) const;
  [[nodiscard]] bool hasValidTitle(int index) const;
  [[nodiscard]] int categoryIndex(int index) const;
  [[nodiscard]] const QList<CategoryRule> &rules() const;
  [[nodiscard]] bool isEmpty() const;

private:
  struct Compiled {
    QString category;
    WindowMatcher matcher;
    int categoryIndex = -1;
  };

  bool isIndex(int index) const;

private:
  QList<CategoryRule> _rules;
  QList<Compiled> _compiled;
  QStringList _categoryNames;
};

struct CategoryTotal {
  // Empty for the time no rule claimed.
  QString category;
  qint64 seconds = 0;
  int sessionCount = 0;
};

QList<CategoryTotal> categoryTotals(const QList<TitleTotal> &titles,
                                    const CategoryRules &rules);

QString serializeCategoryRules(const QList<CategoryRule> &rules);
QList<CategoryRule> parseCategoryRules(const QString &json);

} // namespace chronexa::activity
