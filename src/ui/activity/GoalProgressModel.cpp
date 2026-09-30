#include "GoalProgressModel.hpp"
#include "ActivityFormat.hpp"

#include <algorithm>

namespace chronexa::activity {

GoalProgressModel::GoalProgressModel(QObject *parent)
    : QAbstractListModel(parent) {}

int GoalProgressModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _progress.size();
}

QVariant GoalProgressModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 || index.row() >= _progress.size()) {
    return {};
  }

  const GoalProgress &item = _progress.at(index.row());
  switch (role) {
  case CategoryRole:
  case Qt::DisplayRole:
    return item.goal.category;
  case KindRole:
    return goalKindKey(item.goal.kind);
  case KindTextRole:
    return format::goalKind(item.goal.kind);
  case SecondsRole:
    return item.seconds;
  case ThresholdSecondsRole:
    return item.goal.thresholdSeconds();
  case FractionRole:
    return item.fraction();
  case StateRole:
    return stateKey(item.state());
  case AmountTextRole:
    return format::goalAmount(item);
  case StatusTextRole:
    return format::goalStatus(item);
  case ColorSlotRole:
    return static_cast<int>(_colorOrder.indexOf(item.goal.category));
  default:
    return {};
  }
}

QHash<int, QByteArray> GoalProgressModel::roleNames() const {
  return {
      {CategoryRole, "category"},
      {KindRole, "kind"},
      {KindTextRole, "kindText"},
      {SecondsRole, "seconds"},
      {ThresholdSecondsRole, "thresholdSeconds"},
      {FractionRole, "fraction"},
      {StateRole, "goalState"},
      {AmountTextRole, "amountText"},
      {StatusTextRole, "statusText"},
      {ColorSlotRole, "colorSlot"},
  };
}

void GoalProgressModel::setProgress(QList<GoalProgress> progress) {
  // Same goals, new times: the common minute-by-minute case keeps its rows,
  // so the bars animate instead of the list being rebuilt.
  const bool sameGoals =
      progress.size() == _progress.size() &&
      std::equal(progress.cbegin(), progress.cend(), _progress.cbegin(),
                 [](const GoalProgress &a, const GoalProgress &b) {
                   return a.goal == b.goal;
                 });
  if (sameGoals) {
    _progress = std::move(progress);
    if (!_progress.isEmpty()) {
      emit dataChanged(index(0), index(_progress.size() - 1));
    }
    return;
  }

  const qsizetype before = _progress.size();
  beginResetModel();
  _progress = std::move(progress);
  endResetModel();
  if (before != _progress.size()) {
    emit countChanged();
  }
}

void GoalProgressModel::setColorOrder(QStringList categoryNames) {
  _colorOrder = std::move(categoryNames);
  retranslate();
}

void GoalProgressModel::retranslate() {
  if (!_progress.isEmpty()) {
    emit dataChanged(index(0), index(_progress.size() - 1));
  }
}

QString GoalProgressModel::stateKey(GoalState state) {
  switch (state) {
  case GoalState::Within:
    return QStringLiteral("within");
  case GoalState::Exceeded:
    return QStringLiteral("exceeded");
  case GoalState::Short:
    return QStringLiteral("short");
  case GoalState::Reached:
    return QStringLiteral("reached");
  }
  return {};
}

} // namespace chronexa::activity
