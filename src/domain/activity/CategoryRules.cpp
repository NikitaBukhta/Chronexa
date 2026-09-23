#include "CategoryRules.hpp"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace {

constexpr auto kCategoryKey = "category";
constexpr auto kAppsKey = "apps";
constexpr auto kTitleKey = "title";

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

QRegularExpression titleExpression(const QString &pattern) {
  return QRegularExpression(pattern,
                            QRegularExpression::CaseInsensitiveOption |
                                QRegularExpression::UseUnicodePropertiesOption);
}

} // namespace

namespace chronexa::activity {

bool CategoryRule::operator==(const CategoryRule &other) const {
  return category == other.category && apps == other.apps &&
         titlePattern == other.titlePattern;
}

bool CategoryRule::operator!=(const CategoryRule &other) const {
  return !(*this == other);
}

bool CategoryRule::isValid() const {
  return CategoryRules({*this}).isUsable(0);
}

CategoryRules::CategoryRules(QList<CategoryRule> rules)
    : _rules(std::move(rules)) {
  _compiled.reserve(_rules.size());
  for (const CategoryRule &rule : std::as_const(_rules)) {
    Compiled compiled;
    compiled.category = rule.category.trimmed();
    compiled.apps = normalizedApps(rule.apps);
    compiled.checksTitle = !rule.titlePattern.trimmed().isEmpty();
    if (compiled.checksTitle) {
      compiled.title = titleExpression(rule.titlePattern);
      compiled.titleValid = compiled.title.isValid();
    }

    // Without any condition a rule would claim every window.
    const bool usable = !compiled.category.isEmpty() &&
                        (!compiled.apps.isEmpty() || compiled.checksTitle) &&
                        compiled.titleValid;
    if (usable) {
      compiled.categoryIndex = _categoryNames.indexOf(compiled.category);
      if (compiled.categoryIndex < 0) {
        compiled.categoryIndex = _categoryNames.size();
        _categoryNames.append(compiled.category);
      }
      if (compiled.checksTitle) {
        compiled.title.optimize();
      }
    }
    _compiled.append(std::move(compiled));
  }
}

QString CategoryRules::categorize(const QString &appName,
                                  const QString &title) const {
  for (const Compiled &rule : _compiled) {
    if (rule.categoryIndex < 0) {
      continue;
    }
    const bool appMatches =
        rule.apps.isEmpty() ||
        std::any_of(rule.apps.cbegin(), rule.apps.cend(),
                    [&appName](const QString &app) {
                      return appName.contains(app, Qt::CaseInsensitive);
                    });
    if (!appMatches) {
      continue;
    }
    if (rule.checksTitle && !rule.title.match(title).hasMatch()) {
      continue;
    }
    return rule.category;
  }
  return {};
}

const QStringList &CategoryRules::categoryNames() const {
  return _categoryNames;
}

bool CategoryRules::isIndex(int index) const {
  return index >= 0 && index < _compiled.size();
}

bool CategoryRules::isUsable(int index) const {
  return categoryIndex(index) >= 0;
}

bool CategoryRules::hasValidTitle(int index) const {
  return isIndex(index) && _compiled.at(index).titleValid;
}

int CategoryRules::categoryIndex(int index) const {
  return isIndex(index) ? _compiled.at(index).categoryIndex : -1;
}

const QList<CategoryRule> &CategoryRules::rules() const { return _rules; }

bool CategoryRules::isEmpty() const { return _rules.isEmpty(); }

QList<CategoryTotal> categoryTotals(const QList<TitleTotal> &titles,
                                    const CategoryRules &rules) {
  struct Sum {
    qint64 milliseconds = 0;
    int sessionCount = 0;
  };

  // Categorized per distinct (app, title) rather than per session: a regex run
  // for every row of a month would be the expensive part of the query.
  QHash<QString, Sum> sums;
  for (const TitleTotal &title : titles) {
    Sum &sum = sums[rules.categorize(title.appName, title.title)];
    sum.milliseconds += title.milliseconds;
    sum.sessionCount += title.sessionCount;
  }

  QList<CategoryTotal> result;
  result.reserve(sums.size());
  for (auto it = sums.cbegin(); it != sums.cend(); ++it) {
    result.append({it.key(), it->milliseconds / 1000, it->sessionCount});
  }

  std::sort(result.begin(), result.end(),
            [](const CategoryTotal &a, const CategoryTotal &b) {
              if (a.category.isEmpty() != b.category.isEmpty()) {
                return b.category.isEmpty();
              }
              if (a.seconds != b.seconds) {
                return a.seconds > b.seconds;
              }
              return a.category < b.category;
            });
  return result;
}

QString serializeCategoryRules(const QList<CategoryRule> &rules) {
  QJsonArray array;
  for (const CategoryRule &rule : rules) {
    QJsonObject object;
    object.insert(QLatin1String(kCategoryKey), rule.category);
    object.insert(QLatin1String(kAppsKey),
                  QJsonArray::fromStringList(rule.apps));
    object.insert(QLatin1String(kTitleKey), rule.titlePattern);
    array.append(object);
  }
  return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<CategoryRule> parseCategoryRules(const QString &json) {
  QList<CategoryRule> rules;
  const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
  if (!document.isArray()) {
    return rules;
  }

  const QJsonArray array = document.array();
  for (const QJsonValue &value : array) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject object = value.toObject();
    CategoryRule rule;
    rule.category = object.value(QLatin1String(kCategoryKey)).toString();
    const QJsonArray apps = object.value(QLatin1String(kAppsKey)).toArray();
    for (const QJsonValue &app : apps) {
      rule.apps.append(app.toString());
    }
    rule.titlePattern = object.value(QLatin1String(kTitleKey)).toString();
    rules.append(rule);
  }
  return rules;
}

} // namespace chronexa::activity
