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
    CategoryRole,
    CategoryModeRole,
  };

  explicit UserActivityModel(QObject *parent = nullptr);

  [[nodiscard]] int
  rowCount(const QModelIndex &parent = QModelIndex()) const override;
  [[nodiscard]] QVariant data(const QModelIndex &index,
                              int role = Qt::DisplayRole) const override;
  [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
  void setActivities(QList<Activity> activities, QStringList categories = {});

  void setColorOrder(const QStringList &appNames);

public slots:
  void clear();

signals:
  void countChanged();

private:
  QList<Activity> _activities;
  QStringList _categories;
  QStringList _colorOrder;
};

} // namespace chronexa::activity
