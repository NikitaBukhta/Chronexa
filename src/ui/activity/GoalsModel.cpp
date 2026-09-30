#include "GoalsModel.hpp"
#include "CategoryRulesModel.hpp"

#include "core/AppSettings.hpp"

#include <QLoggingCategory>

#include <algorithm>

namespace {

Q_LOGGING_CATEGORY(lcGoalsModel, "chronexa.activity.goalsmodel")

constexpr int kDefaultLimitMinutes = 60;
constexpr int kDefaultTargetMinutes = 4 * 60;

} // namespace

namespace chronexa::activity {

GoalsModel::GoalsModel(core::AppSettings *settings, QObject *parent)
    : QAbstractListModel(parent), _settings(settings),
      _categories(settings->categoryRules()) {
  if (!_settings->hasDailyGoals()) {
    qCInfo(lcGoalsModel) << "No goals saved -- seeding the defaults";
    _settings->setDailyGoals(defaultGoals(_settings->categoryRules()));
  }
  _goals = _settings->dailyGoals();

  connect(_settings, &core::AppSettings::categoryRulesChanged, this,
          &GoalsModel::onCategoryRulesChanged);
}

QList<DailyGoal> GoalsModel::defaultGoals(const QList<CategoryRule> &rules) {
  // The names come from the default rules, so they match in any language the
  // rules were seeded in.
  const QList<CategoryRule> defaults = CategoryRulesModel::defaultRules();
  const QStringList existing = CategoryRules(rules).categoryNames();

  QList<DailyGoal> goals;
  const auto offer = [&](int defaultIndex, GoalKind kind, int minutes,
                         int days) {
    if (defaultIndex >= defaults.size()) {
      return;
    }
    const QString category = defaults.at(defaultIndex).category;
    if (existing.contains(category)) {
      goals.append({category, kind, minutes, days});
    }
  };
  offer(0, GoalKind::Limit, kDefaultLimitMinutes, TrackingSchedule::kEveryDay);
  offer(1, GoalKind::Target, kDefaultTargetMinutes,
        TrackingSchedule::kWorkdays);
  return goals;
}

int GoalsModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _goals.size();
}

QVariant GoalsModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || !isRow(index.row())) {
    return {};
  }

  const DailyGoal &goal = _goals.at(index.row());
  switch (role) {
  case CategoryRole:
  case Qt::DisplayRole:
    return goal.category;
  case KindRole:
    return goalKindKey(goal.kind);
  case MinutesRole:
    return goal.minutes;
  case DaysRole:
    return goal.days;
  case ValidRole:
    return goal.isValid();
  case KnownCategoryRole:
    return _categories.categoryNames().contains(goal.category);
  case ColorSlotRole:
    return static_cast<int>(_categories.categoryNames().indexOf(goal.category));
  default:
    return {};
  }
}

QHash<int, QByteArray> GoalsModel::roleNames() const {
  return {
      {CategoryRole, "category"},   {KindRole, "kind"},
      {MinutesRole, "minutes"},     {DaysRole, "days"},
      {ValidRole, "valid"},         {KnownCategoryRole, "knownCategory"},
      {ColorSlotRole, "colorSlot"},
  };
}

QStringList GoalsModel::categoryNames() const {
  return _categories.categoryNames();
}

void GoalsModel::addGoal() {
  DailyGoal goal;
  // The first category not yet limited is the likeliest next pick.
  const QStringList names = _categories.categoryNames();
  for (const QString &name : names) {
    const bool taken =
        std::any_of(_goals.cbegin(), _goals.cend(),
                    [&name](const DailyGoal &g) { return g.category == name; });
    if (!taken) {
      goal.category = name;
      break;
    }
  }
  if (goal.category.isEmpty() && !names.isEmpty()) {
    goal.category = names.first();
  }

  const int row = _goals.size();
  beginInsertRows(QModelIndex(), row, row);
  _goals.append(goal);
  endInsertRows();
  emit countChanged();
  _settings->setDailyGoals(_goals);
}

void GoalsModel::removeGoal(int row) {
  if (!isRow(row)) {
    return;
  }
  beginRemoveRows(QModelIndex(), row, row);
  _goals.removeAt(row);
  endRemoveRows();
  emit countChanged();
  _settings->setDailyGoals(_goals);
}

void GoalsModel::setCategory(int row, const QString &category) {
  editRow(row,
          [&category](DailyGoal &goal) { goal.category = category.trimmed(); });
}

void GoalsModel::setKind(int row, const QString &kind) {
  editRow(row, [&kind](DailyGoal &goal) { goal.kind = goalKindFromKey(kind); });
}

void GoalsModel::setMinutes(int row, int minutes) {
  editRow(row, [minutes](DailyGoal &goal) {
    goal.minutes = qBound(0, minutes, DailyGoal::kMaxMinutes);
  });
}

void GoalsModel::toggleDay(int row, int dayOfWeek) {
  if (dayOfWeek < 1 || dayOfWeek > 7) {
    return;
  }
  editRow(row,
          [dayOfWeek](DailyGoal &goal) { goal.days ^= 1 << (dayOfWeek - 1); });
}

bool GoalsModel::isRow(int row) const {
  return row >= 0 && row < _goals.size();
}

void GoalsModel::editRow(int row,
                         const std::function<void(DailyGoal &)> &edit) {
  if (!isRow(row)) {
    return;
  }
  DailyGoal edited = _goals.at(row);
  edit(edited);
  if (edited == _goals.at(row)) {
    return;
  }
  _goals[row] = std::move(edited);
  emit dataChanged(index(row), index(row));
  _settings->setDailyGoals(_goals);
}

void GoalsModel::onCategoryRulesChanged() {
  // A rename in the rules does not follow into the goals: the goal is then
  // flagged, and the user picks the new name.
  _categories = CategoryRules(_settings->categoryRules());
  emit categoryNamesChanged();
  if (!_goals.isEmpty()) {
    emit dataChanged(index(0), index(_goals.size() - 1));
  }
}

} // namespace chronexa::activity
