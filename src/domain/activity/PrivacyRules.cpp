#include "PrivacyRules.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

constexpr auto kActionKey = "action";
constexpr auto kAppsKey = "apps";
constexpr auto kTitleKey = "title";

constexpr auto kExclude = "exclude";
constexpr auto kHideTitle = "hideTitle";

} // namespace

namespace chronexa::activity {

bool PrivacyRule::operator==(const PrivacyRule &other) const {
  return action == other.action && apps == other.apps &&
         titlePattern == other.titlePattern;
}

bool PrivacyRule::operator!=(const PrivacyRule &other) const {
  return !(*this == other);
}

bool PrivacyRule::isValid() const { return PrivacyRules({*this}).isUsable(0); }

PrivacyRules::PrivacyRules(QList<PrivacyRule> rules)
    : _rules(std::move(rules)) {
  _matchers.reserve(_rules.size());
  for (const PrivacyRule &rule : std::as_const(_rules)) {
    _matchers.append(WindowMatcher(rule.apps, rule.titlePattern));
  }
}

Privacy PrivacyRules::classify(const QString &appName,
                               const QString &title) const {
  Privacy result = Privacy::Record;
  for (int i = 0; i < _rules.size(); ++i) {
    const Privacy action = _rules.at(i).action;
    // Nothing a later rule says can make the verdict stricter than this.
    if (action <= result || !isUsable(i)) {
      continue;
    }
    if (_matchers.at(i).matches(appName, title)) {
      result = action;
      if (result == Privacy::Exclude) {
        break;
      }
    }
  }
  return result;
}

bool PrivacyRules::isIndex(int index) const {
  return index >= 0 && index < _rules.size();
}

bool PrivacyRules::isUsable(int index) const {
  return isIndex(index) && _rules.at(index).action != Privacy::Record &&
         _matchers.at(index).isUsable();
}

bool PrivacyRules::hasValidTitle(int index) const {
  return isIndex(index) && _matchers.at(index).isTitleValid();
}

const QList<PrivacyRule> &PrivacyRules::rules() const { return _rules; }

bool PrivacyRules::isEmpty() const { return _rules.isEmpty(); }

QString privacyKey(Privacy action) {
  switch (action) {
  case Privacy::Exclude:
    return QLatin1String(kExclude);
  case Privacy::HideTitle:
    return QLatin1String(kHideTitle);
  case Privacy::Record:
    break;
  }
  return {};
}

Privacy privacyFromKey(const QString &key) {
  if (key == QLatin1String(kExclude)) {
    return Privacy::Exclude;
  }
  if (key == QLatin1String(kHideTitle)) {
    return Privacy::HideTitle;
  }
  return Privacy::Record;
}

QString serializePrivacyRules(const QList<PrivacyRule> &rules) {
  QJsonArray array;
  for (const PrivacyRule &rule : rules) {
    QJsonObject object;
    object.insert(QLatin1String(kActionKey), privacyKey(rule.action));
    object.insert(QLatin1String(kAppsKey),
                  QJsonArray::fromStringList(rule.apps));
    object.insert(QLatin1String(kTitleKey), rule.titlePattern);
    array.append(object);
  }
  return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<PrivacyRule> parsePrivacyRules(const QString &json) {
  QList<PrivacyRule> rules;
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
    PrivacyRule rule;
    rule.action =
        privacyFromKey(object.value(QLatin1String(kActionKey)).toString());
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
