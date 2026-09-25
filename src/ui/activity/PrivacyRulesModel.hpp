#pragma once

#include "domain/activity/PrivacyRules.hpp"

#include <QAbstractListModel>
#include <QList>

#include <functional>

namespace chronexa::core {

class AppSettings;

} // namespace chronexa::core

namespace chronexa::activity {

// The editable list of privacy rules behind the settings page. Saved on every
// edit, like CategoryRulesModel; unordered, since the strictest match wins.
class PrivacyRulesModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
  enum Role {
    // privacyKey() of the action: "exclude" or "hideTitle".
    ActionRole = Qt::UserRole + 1,
    AppsRole,
    TitlePatternRole,
    TitleValidRole,
    ValidRole,
  };

  explicit PrivacyRulesModel(core::AppSettings *settings,
                             QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  Q_INVOKABLE void addRule();
  Q_INVOKABLE void removeRule(int row);
  Q_INVOKABLE void restoreDefaults();

  // An unknown key is ignored rather than stored as a rule that does nothing.
  Q_INVOKABLE void setAction(int row, const QString &action);
  Q_INVOKABLE void setApps(int row, const QString &apps);
  Q_INVOKABLE void setTitlePattern(int row, const QString &pattern);

  static QList<PrivacyRule> defaultRules();

signals:
  void countChanged();

private:
  bool isRow(int row) const;
  void editRow(int row, const std::function<void(PrivacyRule &)> &edit);
  void save();

  core::AppSettings *_settings = nullptr;
  QList<PrivacyRule> _rules;
  // Compiled once per change; data() runs on every repaint.
  PrivacyRules _analysis;
};

} // namespace chronexa::activity
