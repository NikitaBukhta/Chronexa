#pragma once

#include "domain/activity/DailyGoals.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

namespace chronexa::activity {

// Today's goals with how far each has come, for the day page.
class GoalProgressModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Role {
    CategoryRole = Qt::UserRole + 1,
    KindRole,
    KindTextRole,
    SecondsRole,
    ThresholdSecondsRole,
    FractionRole,
    StateRole,
    AmountTextRole,
    StatusTextRole,
    ColorSlotRole,
  };

  explicit GoalProgressModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  void setProgress(QList<GoalProgress> progress);
  // Category names in rule order: a goal's colour is its category's.
  void setColorOrder(QStringList categoryNames);
  // Re-renders the text roles, e.g. after a language switch.
  void retranslate();

  static QString stateKey(GoalState state);

signals:
  void countChanged();

private:
  QList<GoalProgress> _progress;
  QStringList _colorOrder;
};

} // namespace chronexa::activity
