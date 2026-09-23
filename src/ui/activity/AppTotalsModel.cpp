#include "AppTotalsModel.hpp"

#include "ActivityFormat.hpp"

namespace chronexa::activity {

AppTotalsModel::AppTotalsModel(QObject *parent) : QAbstractListModel(parent) {}

int AppTotalsModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _totals.size();
}

qint64 AppTotalsModel::totalSeconds() const { return _totalSeconds; }

QVariant AppTotalsModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return {};
  }
  const int row = index.row();
  if (row < 0 || row >= _totals.size()) {
    return {};
  }

  const AppTotal &total = _totals.at(row);
  switch (role) {
  case AppNameRole:
  case Qt::DisplayRole:
    return total.appName;
  case SecondsRole:
    return total.seconds;
  case SessionCountRole:
    return total.sessionCount;
  case DurationTextRole:
    return format::duration(total.seconds);
  case ShareRole:
    return _totalSeconds > 0
               ? static_cast<double>(total.seconds) / _totalSeconds
               : 0.0;
  case SharePercentTextRole: {
    if (_totalSeconds <= 0) {
      return QStringLiteral("0%");
    }
    const double percent = 100.0 * total.seconds / _totalSeconds;
    return percent < 1.0 ? QStringLiteral("%1%").arg(percent, 0, 'f', 1)
                         : QStringLiteral("%1%").arg(qRound(percent));
  }
  case ColorSlotRole:
    return _colorOrder.indexOf(total.appName);
  default:
    return {};
  }
}

QHash<int, QByteArray> AppTotalsModel::roleNames() const {
  return {
      {AppNameRole, "appName"},
      {SecondsRole, "seconds"},
      {SessionCountRole, "sessionCount"},
      {DurationTextRole, "durationText"},
      {ShareRole, "share"},
      {SharePercentTextRole, "sharePercentText"},
      {ColorSlotRole, "colorSlot"},
  };
}

void AppTotalsModel::setTotals(QList<AppTotal> totals) {
  beginResetModel();
  _totals = std::move(totals);
  _totalSeconds = 0;
  for (const AppTotal &total : _totals) {
    _totalSeconds += total.seconds;
  }
  endResetModel();
  emit countChanged();
}

void AppTotalsModel::setColorOrder(const QStringList &appNames) {
  if (_colorOrder == appNames) {
    return;
  }
  _colorOrder = appNames;
  if (!_totals.isEmpty()) {
    emit dataChanged(index(0), index(_totals.size() - 1), {ColorSlotRole});
  }
}

} // namespace chronexa::activity
