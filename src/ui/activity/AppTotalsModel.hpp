#pragma once

#include "domain/activity/ActivityStats.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

namespace chronexa::activity {

class AppTotalsModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(qint64 totalSeconds READ totalSeconds NOTIFY countChanged)

public:
  enum Role {
    AppNameRole = Qt::UserRole + 1,
    SecondsRole,
    SessionCountRole,
    DurationTextRole,
    ShareRole,
    SharePercentTextRole,
    ColorSlotRole,
  };

  explicit AppTotalsModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  qint64 totalSeconds() const;

  void setTotals(QList<AppTotal> totals);
  void setColorOrder(const QStringList &appNames);

signals:
  void countChanged();

private:
  QList<AppTotal> _totals;
  QStringList _colorOrder;
  qint64 _totalSeconds = 0;
};

} // namespace chronexa::activity
