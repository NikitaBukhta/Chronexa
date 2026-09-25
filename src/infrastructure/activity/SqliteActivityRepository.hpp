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
  std::optional<QList<WindowRef>> windows() const override;
  int redact(const QList<WindowRef> &remove,
             const QList<WindowRef> &hideTitle) override;

  QList<Activity> sessions(const QDateTime &from, const QDateTime &to,
                           int limit = -1) const override;
  QList<AppTotal> appTotals(const QDateTime &from,
                            const QDateTime &to) const override;
  QList<TitleTotal> titleTotals(const QDateTime &from,
                                const QDateTime &to) const override;
  QList<Interval> intervals(const QDateTime &from,
                            const QDateTime &to) const override;
  RangeStats stats(const QDateTime &from, const QDateTime &to) const override;
  QPair<QDateTime, QDateTime> bounds() const override;
  QStringList rankedAppNames(int limit) const override;

private:
  bool createSchema();
  // Moves the WAL into the main file and truncates it, so pages holding
  // deleted text do not linger in chronexa.db-wal.
  void checkpoint();

  QString _databasePath;
  QString _connectionName;
  QSqlDatabase _db;
};

} // namespace chronexa::activity
