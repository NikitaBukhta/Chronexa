#pragma once

#include "domain/activity/Activity.hpp"

#include <QAbstractListModel>
#include <QList>

namespace chronexa::activity {

class UserActivityModel : public QAbstractListModel {
  Q_OBJECT

public:
  enum Role {
    AppNameRole = Qt::UserRole + 1,
    TitleRole,
    StartedOnRole,
    EndedOnRole,
    DurationSecondsRole,
  };

  explicit UserActivityModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

public slots:
  void onActivitiesReceived(QList<Activity> batch);
  void clear();

private:
  QList<Activity> _activities;
};

} // namespace chronexa::activity
