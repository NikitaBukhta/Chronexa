#pragma once

#include "IActivityRepository.hpp"

#include <QSqlDatabase>
#include <QString>

namespace chronexa::activity {

class SqliteActivityRepository : public IActivityRepository {
public:
  explicit SqliteActivityRepository(QString databasePath);
  ~SqliteActivityRepository() override;

  bool open() override;
  bool insertBatch(const QList<Activity> &activities) override;
  bool clearAll() override;
  [[nodiscard]] std::optional<QList<WindowRef>> windows() const override;
  int redact(const QList<WindowRef> &remove,
             const QList<WindowRef> &hideTitle) override;
  int editSessions(const WindowRef &window, const QDateTime &from,
                   const QDateTime &to, const SessionEdit &edit) override;
  int cutSessions(const WindowRef &window, const QDateTime &from,
                  const QDateTime &to) override;

  [[nodiscard]] QList<Activity> sessions(const QDateTime &from,
                                         const QDateTime &to,
                                         int limit = -1) const override;
  [[nodiscard]] QList<AppTotal> appTotals(const QDateTime &from,
                                          const QDateTime &to) const override;
  [[nodiscard]] QList<TitleTotal>
  titleTotals(const QDateTime &from, const QDateTime &to) const override;
  [[nodiscard]] QList<Interval> intervals(const QDateTime &from,
                                          const QDateTime &to) const override;
  [[nodiscard]] RangeStats stats(const QDateTime &from,
                                 const QDateTime &to) const override;
  [[nodiscard]] QPair<QDateTime, QDateTime> bounds() const override;
  [[nodiscard]] QStringList rankedAppNames(int limit) const override;

private:
  bool createSchema();
  bool migrate();
  [[nodiscard]] bool hasColumn(const QString &column) const;
  void checkpoint();

  QString _databasePath;
  QString _connectionName;
  QSqlDatabase _db;
};

} // namespace chronexa::activity
