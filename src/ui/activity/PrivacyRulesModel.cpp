#include "PrivacyRulesModel.hpp"

#include "ActivityFormat.hpp"
#include "core/AppSettings.hpp"

#include <QLoggingCategory>

namespace {

Q_LOGGING_CATEGORY(lcPrivacy, "chronexa.activity.privacy")

} // namespace

namespace chronexa::activity {

PrivacyRulesModel::PrivacyRulesModel(core::AppSettings *settings,
                                     QObject *parent)
    : QAbstractListModel(parent), _settings(settings) {
  if (!_settings->hasPrivacyRules()) {
    qCInfo(lcPrivacy) << "No privacy rules saved -- seeding the defaults";
    _settings->setPrivacyRules(defaultRules());
  }
  _rules = _settings->privacyRules();
  _analysis = PrivacyRules(_rules);
}

QList<PrivacyRule> PrivacyRulesModel::defaultRules() {
  // Names are the executables' descriptions, matched as substrings, so
  // "KeePass" also covers KeePassXC.
  return {
      {Privacy::Exclude,
       {QStringLiteral("KeePass"), QStringLiteral("1Password"),
        QStringLiteral("Bitwarden"), QStringLiteral("LastPass")},
       {}},
      {Privacy::Exclude,
       {QStringLiteral("Chrome"), QStringLiteral("Firefox"),
        QStringLiteral("Edge"), QStringLiteral("Brave"),
        QStringLiteral("Opera")},
       // Browsers translate the marker, so the app's own languages are here.
       QStringLiteral("Incognito|InPrivate|Private Browsing|Инкогнито|"
                      "Приватный просмотр|Приватний перегляд|Анонімн")},
      {Privacy::HideTitle,
       {QStringLiteral("Telegram"), QStringLiteral("WhatsApp"),
        QStringLiteral("Signal"), QStringLiteral("Viber"),
        QStringLiteral("Discord"), QStringLiteral("Messenger")},
       {}},
  };
}

int PrivacyRulesModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _rules.size();
}

QVariant PrivacyRulesModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !isRow(index.row())) {
    return {};
  }

  const int row = index.row();
  const PrivacyRule &rule = _rules.at(row);
  switch (role) {
  case ActionRole:
    return privacyKey(rule.action);
  case AppsRole:
  case Qt::DisplayRole:
    return format::joinApps(rule.apps);
  case TitlePatternRole:
    return rule.titlePattern;
  case TitleValidRole:
    return _analysis.hasValidTitle(row);
  case ValidRole:
    return _analysis.isUsable(row);
  default:
    return {};
  }
}

QHash<int, QByteArray> PrivacyRulesModel::roleNames() const {
  return {
      {ActionRole, "action"},
      {AppsRole, "apps"},
      {TitlePatternRole, "titlePattern"},
      {TitleValidRole, "titleValid"},
      {ValidRole, "valid"},
  };
}

void PrivacyRulesModel::addRule() {
  const int row = _rules.size();
  beginInsertRows(QModelIndex(), row, row);
  _rules.append(PrivacyRule());
  endInsertRows();
  emit countChanged();
  save();
}

void PrivacyRulesModel::removeRule(int row) {
  if (!isRow(row)) {
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  _rules.removeAt(row);
  endRemoveRows();
  emit countChanged();
  save();
}

void PrivacyRulesModel::restoreDefaults() {
  beginResetModel();
  _rules = defaultRules();
  endResetModel();
  emit countChanged();
  save();
}

void PrivacyRulesModel::setAction(int row, const QString &action) {
  const Privacy parsed = privacyFromKey(action);
  if (parsed == Privacy::Record) {
    qCWarning(lcPrivacy) << "Unknown privacy action" << action;
    return;
  }
  editRow(row, [parsed](PrivacyRule &rule) { rule.action = parsed; });
}

void PrivacyRulesModel::setApps(int row, const QString &apps) {
  editRow(row,
          [&apps](PrivacyRule &rule) { rule.apps = format::splitApps(apps); });
}

void PrivacyRulesModel::setTitlePattern(int row, const QString &pattern) {
  editRow(row, [&pattern](PrivacyRule &rule) {
    rule.titlePattern = pattern.trimmed();
  });
}

bool PrivacyRulesModel::isRow(int row) const {
  return row >= 0 && row < _rules.size();
}

void PrivacyRulesModel::editRow(
    int row, const std::function<void(PrivacyRule &)> &edit) {
  if (!isRow(row)) {
    return;
  }
  PrivacyRule edited = _rules.at(row);
  edit(edited);
  if (edited == _rules.at(row)) {
    return;
  }
  _rules[row] = std::move(edited);
  // Rules do not affect each other here, so only the edited row changes.
  _analysis = PrivacyRules(_rules);
  emit dataChanged(index(row), index(row));
  _settings->setPrivacyRules(_rules);
}

void PrivacyRulesModel::save() {
  _analysis = PrivacyRules(_rules);
  if (!_rules.isEmpty()) {
    emit dataChanged(index(0), index(_rules.size() - 1));
  }
  _settings->setPrivacyRules(_rules);
}

} // namespace chronexa::activity
