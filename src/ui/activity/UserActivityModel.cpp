#include "UserActivityModel.hpp"

#include "ActivityFormat.hpp"

namespace chronexa::activity {

UserActivityModel::UserActivityModel(QObject *parent)
    : QAbstractListModel(parent) {}

int UserActivityModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _activities.size();
}

QVariant UserActivityModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return {};
  }
  const int row = index.row();
  if (row < 0 || row >= _activities.size()) {
    return {};
  }

  const Activity &activity = _activities.at(row);
  switch (role) {
  case AppNameRole:
  case Qt::DisplayRole:
    return activity.appName;
  case TitleRole:
    return activity.title;
  case StartedOnRole:
    return activity.startedOn;
  case EndedOnRole:
    return activity.endedOn;
  case DurationSecondsRole:
    return activity.durationSeconds();
  case DurationTextRole:
    return format::duration(activity.durationSeconds());
  case StartedTextRole:
    return format::clock(activity.startedOn);
  case EndedTextRole:
    return format::clock(activity.endedOn);
  case DayTextRole:
    return format::dayLabel(activity.startedOn.date());
  case ColorSlotRole:
    return _colorOrder.indexOf(activity.appName);
  case CategoryRole:
    return _categories.value(row);
  case CategoryModeRole:
    return !activity.category             ? QStringLiteral("auto")
           : activity.category->isEmpty() ? QStringLiteral("none")
                                          : QStringLiteral("set");
  default:
    return {};
  }
}

QHash<int, QByteArray> UserActivityModel::roleNames() const {
  return {
      {AppNameRole, "appName"},
      {TitleRole, "title"},
      {StartedOnRole, "startedOn"},
      {EndedOnRole, "endedOn"},
      {DurationSecondsRole, "durationSeconds"},
      {DurationTextRole, "durationText"},
      {StartedTextRole, "startedText"},
      {EndedTextRole, "endedText"},
      {DayTextRole, "dayText"},
      {ColorSlotRole, "colorSlot"},
      {CategoryRole, "category"},
      {CategoryModeRole, "categoryMode"},
  };
}

void UserActivityModel::setActivities(QList<Activity> activities,
                                      QStringList categories) {
  beginResetModel();
  _activities = std::move(activities);
  _categories = std::move(categories);
  endResetModel();
  emit countChanged();
}

void UserActivityModel::setColorOrder(const QStringList &appNames) {
  if (_colorOrder == appNames) {
    return;
  }
  _colorOrder = appNames;
  if (!_activities.isEmpty()) {
    emit dataChanged(index(0), index(_activities.size() - 1), {ColorSlotRole});
  }
}

void UserActivityModel::clear() {
  if (_activities.isEmpty()) {
    return;
  }
  beginResetModel();
  _activities.clear();
  _categories.clear();
  endResetModel();
  emit countChanged();
}

} // namespace chronexa::activity
