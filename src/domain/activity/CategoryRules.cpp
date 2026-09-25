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
    compiled.matcher = WindowMatcher(rule.apps, rule.titlePattern);

    const bool usable =
        !compiled.category.isEmpty() && compiled.matcher.isUsable();
    if (usable) {
      compiled.categoryIndex = _categoryNames.indexOf(compiled.category);
      if (compiled.categoryIndex < 0) {
        compiled.categoryIndex = _categoryNames.size();
        _categoryNames.append(compiled.category);
      }
    }
    _compiled.append(std::move(compiled));
  }
}

QString CategoryRules::categorize(const QString &appName,
                                  const QString &title) const {
  for (const Compiled &rule : _compiled) {
    if (rule.categoryIndex >= 0 && rule.matcher.matches(appName, title)) {
      return rule.category;
    }
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
  return isIndex(index) && _compiled.at(index).matcher.isTitleValid();
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
    auto &[milliseconds, sessionCount] =
        sums[title.category ? *title.category
                            : rules.categorize(title.appName, title.title)];
    milliseconds += title.milliseconds;
    sessionCount += title.sessionCount;
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
