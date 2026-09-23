#pragma once

#include "domain/activity/Activity.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

namespace chronexa::activity {

class UserActivityModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Role {
    AppNameRole = Qt::UserRole + 1,
    TitleRole,
    StartedOnRole,
    EndedOnRole,
    DurationSecondsRole,
    DurationTextRole,
    StartedTextRole,
    EndedTextRole,
    DayTextRole,
    ColorSlotRole,
  };

  explicit UserActivityModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setActivities(QList<Activity> activities);

  void setColorOrder(const QStringList &appNames);

public slots:
  void clear();

signals:
  void countChanged();

private:
  QList<Activity> _activities;
  QStringList _colorOrder;
};

} // namespace chronexa::activity
