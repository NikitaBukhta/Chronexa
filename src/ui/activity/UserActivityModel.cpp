#include "UserActivityModel.hpp"

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

  const Activity &a = _activities.at(row);
  switch (role) {
  case AppNameRole:
  case Qt::DisplayRole:
    return a.appName;
  case TitleRole:
    return a.title;
  case StartedOnRole:
    return a.startedOn;
  case EndedOnRole:
    return a.endedOn;
  case DurationSecondsRole:
    if (a.startedOn.isValid() && a.endedOn.isValid()) {
      return static_cast<qint64>(a.startedOn.secsTo(a.endedOn));
    }
    return 0;
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
  };
}

void UserActivityModel::onActivitiesReceived(QList<Activity> batch) {
  if (batch.isEmpty()) {
    return;
  }

  const int first = _activities.size();
  const int last = first + batch.size() - 1;
  beginInsertRows(QModelIndex(), first, last);

  _activities.append(std::move(batch));

  endInsertRows();
}

void UserActivityModel::clear() {
  if (_activities.isEmpty()) {
    return;
  }
  beginResetModel();
  _activities.clear();
  endResetModel();
}

} // namespace chronexa::activity
