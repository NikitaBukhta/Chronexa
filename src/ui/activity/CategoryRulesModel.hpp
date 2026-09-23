#pragma once

#include "domain/activity/CategoryRules.hpp"

#include <QAbstractListModel>
#include <QList>

#include <functional>

namespace chronexa::core {

class AppSettings;

} // namespace chronexa::core

namespace chronexa::activity {

// The editable, ordered list of category rules behind the settings page.
// Every edit is saved straight away; AppSettings announces it to the rest of
// the app. Rows are updated in place rather than reset, so the field being
// typed into keeps its focus.
class CategoryRulesModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Role {
    CategoryRole = Qt::UserRole + 1,
    AppsRole,
    TitlePatternRole,
    TitleValidRole,
    ValidRole,
    ColorSlotRole,
  };

  explicit CategoryRulesModel(core::AppSettings *settings,
                              QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void addRule();
  Q_INVOKABLE void removeRule(int row);
  Q_INVOKABLE void moveRule(int row, int delta);
  Q_INVOKABLE void restoreDefaults();

  Q_INVOKABLE void setCategory(int row, const QString &category);
  Q_INVOKABLE void setApps(int row, const QString &apps);
  Q_INVOKABLE void setTitlePattern(int row, const QString &pattern);

  static QList<CategoryRule> defaultRules();

  // "CLion, Visual Studio Code" <-> {"CLion", "Visual Studio Code"}.
  static QStringList splitApps(const QString &text);
  static QString joinApps(const QStringList &apps);

signals:
  void countChanged();

private:
  bool isRow(int row) const;
  // Applies an edit to one rule; saves only when it changed something.
  void editRow(int row, const std::function<void(CategoryRule &)> &edit);
  // Re-analyses the rules, announces every row and persists them.
  void save();

  core::AppSettings *_settings = nullptr;
  QList<CategoryRule> _rules;
  // The rules compiled once per change: data() is called on every repaint and
  // must not compile regular expressions.
  CategoryRules _analysis;
};

} // namespace chronexa::activity
