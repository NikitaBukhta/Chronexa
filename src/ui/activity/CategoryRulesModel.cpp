#include "CategoryRulesModel.hpp"

#include "ActivityFormat.hpp"

#include "core/AppSettings.hpp"

#include <QLoggingCategory>

namespace {

Q_LOGGING_CATEGORY(lcCategories, "chronexa.activity.categories")

} // namespace

namespace chronexa::activity {

CategoryRulesModel::CategoryRulesModel(core::AppSettings *settings,
                                       QObject *parent)
    : QAbstractListModel(parent), _settings(settings) {
  if (!_settings->hasCategoryRules()) {
    qCInfo(lcCategories) << "No category rules saved -- seeding the defaults";
    _settings->setCategoryRules(defaultRules());
  }
  _rules = _settings->categoryRules();
  _analysis = CategoryRules(_rules);
}

QList<CategoryRule> CategoryRulesModel::defaultRules() {
  // Narrow before broad: the YouTube rule has to see Chrome first.
  return {
      {tr("Distractions"),
       {QStringLiteral("Chrome"), QStringLiteral("Firefox"),
        QStringLiteral("Edge")},
       QStringLiteral("YouTube|Netflix|Twitch")},
      {tr("Work"),
       {QStringLiteral("CLion"), QStringLiteral("Visual Studio Code")},
       {}},
      {tr("Communication"),
       {QStringLiteral("Slack"), QStringLiteral("Teams"),
        QStringLiteral("Telegram"), QStringLiteral("Zoom")},
       {}},
  };
}

int CategoryRulesModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _rules.size();
}

QVariant CategoryRulesModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !isRow(index.row())) {
    return {};
  }

  const int row = index.row();
  const CategoryRule &rule = _rules.at(row);
  switch (role) {
  case CategoryRole:
  case Qt::DisplayRole:
    return rule.category;
  case AppsRole:
    return format::joinApps(rule.apps);
  case TitlePatternRole:
    return rule.titlePattern;
  case TitleValidRole:
    return _analysis.hasValidTitle(row);
  case ValidRole:
    return _analysis.isUsable(row);
  case ColorSlotRole:
    return _analysis.categoryIndex(row);
  default:
    return {};
  }
}

QHash<int, QByteArray> CategoryRulesModel::roleNames() const {
  return {
      {CategoryRole, "category"},
      {AppsRole, "apps"},
      {TitlePatternRole, "titlePattern"},
      {TitleValidRole, "titleValid"},
      {ValidRole, "valid"},
      {ColorSlotRole, "colorSlot"},
  };
}

void CategoryRulesModel::addRule() {
  const int row = _rules.size();
  beginInsertRows(QModelIndex(), row, row);
  _rules.append(CategoryRule());
  endInsertRows();
  emit countChanged();
  save();
}

void CategoryRulesModel::removeRule(int row) {
  if (!isRow(row)) {
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  _rules.removeAt(row);
  endRemoveRows();
  emit countChanged();
  save();
}

void CategoryRulesModel::moveRule(int row, int delta) {
  const int target = row + delta;
  if (!isRow(row) || !isRow(target) || delta == 0) {
    return;
  }
  // beginMoveRows() takes the destination as the row to insert before, which
  // is one past the target when moving down.
  const int destination = delta > 0 ? target + 1 : target;
  if (!beginMoveRows(QModelIndex(), row, row, QModelIndex(), destination)) {
    return;
  }
  _rules.move(row, target);
  endMoveRows();
  save();
}

void CategoryRulesModel::restoreDefaults() {
  beginResetModel();
  _rules = defaultRules();
  endResetModel();
  emit countChanged();
  save();
}

void CategoryRulesModel::setCategory(int row, const QString &category) {
  editRow(row, [&category](CategoryRule &rule) {
    rule.category = category.trimmed();
  });
}

void CategoryRulesModel::setApps(int row, const QString &apps) {
  editRow(row,
          [&apps](CategoryRule &rule) { rule.apps = format::splitApps(apps); });
}

void CategoryRulesModel::setTitlePattern(int row, const QString &pattern) {
  editRow(row, [&pattern](CategoryRule &rule) {
    rule.titlePattern = pattern.trimmed();
  });
}

bool CategoryRulesModel::isRow(int row) const {
  return row >= 0 && row < _rules.size();
}

void CategoryRulesModel::editRow(
    int row, const std::function<void(CategoryRule &)> &edit) {
  if (!isRow(row)) {
    return;
  }
  CategoryRule edited = _rules.at(row);
  edit(edited);
  if (edited == _rules.at(row)) {
    return;
  }
  _rules[row] = std::move(edited);
  save();
}

void CategoryRulesModel::save() {
  // Renaming, fixing or moving one rule can shift the colour and validity of
  // any other: one notification covers every row, text roles included.
  _analysis = CategoryRules(_rules);
  if (!_rules.isEmpty()) {
    emit dataChanged(index(0), index(_rules.size() - 1));
  }
  _settings->setCategoryRules(_rules);
}

} // namespace chronexa::activity
