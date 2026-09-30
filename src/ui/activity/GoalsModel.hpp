#pragma once

#include "domain/activity/CategoryRules.hpp"
#include "domain/activity/DailyGoals.hpp"

#include <QAbstractListModel>
#include <QList>
#include <QStringList>

#include <functional>

namespace chronexa::core {

class AppSettings;

} // namespace chronexa::core

namespace chronexa::activity {

// The editable list of daily goals behind the settings page. Like
// CategoryRulesModel, every edit is saved straight away and rows are updated
// in place. Goals name a category; one no rule produces is kept but flagged.
class GoalsModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(
      QStringList categoryNames READ categoryNames NOTIFY categoryNamesChanged)

public:
  enum Role {
    CategoryRole = Qt::UserRole + 1,
    KindRole,
    MinutesRole,
    DaysRole,
    ValidRole,
    KnownCategoryRole,
    ColorSlotRole,
  };

  explicit GoalsModel(core::AppSettings *settings, QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  [[nodiscard]] QStringList categoryNames() const;

  Q_INVOKABLE void addGoal();
  Q_INVOKABLE void removeGoal(int row);

  Q_INVOKABLE void setCategory(int row, const QString &category);
  Q_INVOKABLE void setKind(int row, const QString &kind);
  Q_INVOKABLE void setMinutes(int row, int minutes);
  Q_INVOKABLE void toggleDay(int row, int dayOfWeek);

  // The first-run goals, for those of the default categories that exist in
  // `rules`: an hour of distractions at most, four hours of work at least.
  static QList<DailyGoal> defaultGoals(const QList<CategoryRule> &rules);

signals:
  void countChanged();
  void categoryNamesChanged();

private:
  bool isRow(int row) const;
  void editRow(int row, const std::function<void(DailyGoal &)> &edit);
  void onCategoryRulesChanged();

  core::AppSettings *_settings = nullptr;
  QList<DailyGoal> _goals;
  CategoryRules _categories;
};

} // namespace chronexa::activity
