#include "SqliteActivityRepository.hpp"

#include <QDir>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QSqlError>
#include <QSqlQuery>
#include <QTimeZone>
#include <QVariant>

namespace {

Q_LOGGING_CATEGORY(lcRepo, "chronexa.activity.repository")

// The two-argument min()/max() of SQLite are scalar, so a session crossing an
// edge is counted only for the part inside the range.
constexpr auto kClippedStart = "max(started_on, :from)";
constexpr auto kClippedEnd = "min(ended_on, :to)";
constexpr auto kOverlaps = "ended_on > :from AND started_on < :to";

qint64 toMs(const QDateTime &moment) { return moment.toMSecsSinceEpoch(); }

// A null QString binds as SQL NULL, which the NOT NULL title column rejects
// on insert and `title = :title` never matches. A hidden title is exactly
// that null string, so every bound title goes through here.
QString bindableText(const QString &text) {
  return text.isNull() ? QStringLiteral("") : text;
}

QDateTime fromMs(qint64 ms) {
  return QDateTime::fromMSecsSinceEpoch(ms, QTimeZone::LocalTime);
}

} // namespace

namespace chronexa::activity {

SqliteActivityRepository::SqliteActivityRepository(QString databasePath)
    : _databasePath(std::move(databasePath)),
      _connectionName(QStringLiteral("chronexa.activity")) {}

SqliteActivityRepository::~SqliteActivityRepository() {
  if (_db.isOpen()) {
    _db.close();
  }
  _db = QSqlDatabase();
  if (QSqlDatabase::contains(_connectionName)) {
    QSqlDatabase::removeDatabase(_connectionName);
  }
}

bool SqliteActivityRepository::open() {
  if (!QSqlDatabase::isDriverAvailable(QStringLiteral("QSQLITE"))) {
    qCCritical(lcRepo) << "QSQLITE driver unavailable -- history is disabled";
    return false;
  }

  QDir().mkpath(QFileInfo(_databasePath).absolutePath());

  _db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), _connectionName);
  _db.setDatabaseName(_databasePath);
  if (!_db.open()) {
    qCCritical(lcRepo) << "Cannot open" << _databasePath << ":"
                       << _db.lastError().text();
    return false;
  }

  QSqlQuery pragma(_db);
  pragma.exec(QStringLiteral("PRAGMA journal_mode = WAL"));
  pragma.exec(QStringLiteral("PRAGMA synchronous = NORMAL"));
  // Deleted and overwritten rows are zeroed rather than left in free pages:
  // clearing history or hiding a title has to remove the text from the file.
  pragma.exec(QStringLiteral("PRAGMA secure_delete = ON"));

  if (!createSchema()) {
    return false;
  }

  qCInfo(lcRepo) << "History opened at" << _databasePath;
  return true;
}

bool SqliteActivityRepository::createSchema() {
  QSqlQuery query(_db);

  if (!query.exec(QStringLiteral("CREATE TABLE IF NOT EXISTS activity ("
                                 "  id         INTEGER PRIMARY KEY,"
                                 "  app_name   TEXT    NOT NULL,"
                                 "  title      TEXT    NOT NULL,"
                                 "  started_on INTEGER NOT NULL,"
                                 "  ended_on   INTEGER NOT NULL)"))) {
    qCCritical(lcRepo) << "Schema failed:" << query.lastError().text();
    return false;
  }

  query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_activity_span "
                            "ON activity(ended_on, started_on)"));
  query.exec(QStringLiteral("CREATE INDEX IF NOT EXISTS idx_activity_app "
                            "ON activity(app_name)"));
  return true;
}

bool SqliteActivityRepository::insertBatch(const QList<Activity> &activities) {
  if (activities.isEmpty() || !_db.isOpen()) {
    return false;
  }

  _db.transaction();

  QSqlQuery query(_db);
  query.prepare(QStringLiteral(
      "INSERT INTO activity (app_name, title, started_on, ended_on) "
      "VALUES (?, ?, ?, ?)"));

  int written = 0;
  for (const Activity &activity : activities) {
    if (!activity.isValid()) {
      continue;
    }
    query.addBindValue(activity.appName);
    query.addBindValue(bindableText(activity.title));
    query.addBindValue(toMs(activity.startedOn));
    query.addBindValue(toMs(activity.endedOn));
    if (query.exec()) {
      ++written;
    } else {
      qCWarning(lcRepo) << "Insert failed:" << query.lastError().text();
    }
  }

  if (!_db.commit()) {
    _db.rollback();
    qCWarning(lcRepo) << "Commit failed:" << _db.lastError().text();
    return false;
  }

  qCDebug(lcRepo) << "Stored" << written << "of" << activities.size()
                  << "sessions";
  return written > 0;
}

bool SqliteActivityRepository::clearAll() {
  if (!_db.isOpen()) {
    return false;
  }
  QSqlQuery query(_db);
  const bool ok = query.exec(QStringLiteral("DELETE FROM activity"));
  if (!ok) {
    qCWarning(lcRepo) << "Clear failed:" << query.lastError().text();
    return false;
  }
  checkpoint();
  return true;
}

std::optional<QList<WindowRef>> SqliteActivityRepository::windows() const {
  if (!_db.isOpen()) {
    return std::nullopt;
  }

  QSqlQuery query(_db);
  if (!query.exec(
          QStringLiteral("SELECT DISTINCT app_name, title FROM activity"))) {
    qCWarning(lcRepo) << "windows() failed:" << query.lastError().text();
    return std::nullopt;
  }

  QList<WindowRef> result;
  while (query.next()) {
    result.append({query.value(0).toString(), query.value(1).toString()});
  }
  return result;
}

int SqliteActivityRepository::redact(const QList<WindowRef> &remove,
                                     const QList<WindowRef> &hideTitle) {
  if (!_db.isOpen()) {
    return -1;
  }
  if (remove.isEmpty() && hideTitle.isEmpty()) {
    return 0;
  }

  if (!_db.transaction()) {
    qCWarning(lcRepo) << "Redact could not begin:" << _db.lastError().text();
    return -1;
  }

  QSqlQuery removeQuery(_db);
  removeQuery.prepare(QStringLiteral(
      "DELETE FROM activity WHERE app_name = :app AND title = :title"));
  QSqlQuery hideQuery(_db);
  hideQuery.prepare(QStringLiteral("UPDATE activity SET title = '' "
                                   "WHERE app_name = :app AND title = :title"));

  // All or nothing: a half-applied redaction would leave the user believing
  // the text was gone.
  int changed = 0;
  const auto run = [&changed](QSqlQuery &query, const WindowRef &window) {
    query.bindValue(QStringLiteral(":app"), window.appName);
    query.bindValue(QStringLiteral(":title"), bindableText(window.title));
    if (!query.exec()) {
      qCWarning(lcRepo) << "Redact failed:" << query.lastError().text();
      return false;
    }
    changed += query.numRowsAffected();
    return true;
  };

  bool ok = true;
  for (const WindowRef &window : remove) {
    ok = ok && run(removeQuery, window);
  }
  for (const WindowRef &window : hideTitle) {
    if (!window.title.isEmpty()) {
      ok = ok && run(hideQuery, window);
    }
  }

  if (!ok || !_db.commit()) {
    if (ok) {
      qCWarning(lcRepo) << "Redact commit failed:" << _db.lastError().text();
    }
    _db.rollback();
    return -1;
  }

  checkpoint();
  qCInfo(lcRepo) << "Redacted" << changed << "sessions";
  return changed;
}

void SqliteActivityRepository::checkpoint() {
  QSqlQuery query(_db);
  if (!query.exec(QStringLiteral("PRAGMA wal_checkpoint(TRUNCATE)"))) {
    qCWarning(lcRepo) << "Checkpoint failed:" << query.lastError().text();
    return;
  }
  // A busy database is reported in the row, not as an error: another reader
  // keeps the old pages in the WAL until it lets go. SQLite folds the WAL in
  // when the last connection closes, so the text goes at the latest then.
  if (query.next() && query.value(0).toInt() != 0) {
    qCWarning(lcRepo) << "Checkpoint incomplete, the database is in use -- "
                         "removed text stays in the WAL until it is released";
  }
}

QList<Activity> SqliteActivityRepository::sessions(const QDateTime &from,
                                                   const QDateTime &to,
                                                   int limit) const {
  QList<Activity> result;
  if (!_db.isOpen() || !from.isValid() || !to.isValid() || from >= to) {
    return result;
  }

  QSqlQuery query(_db);
  query.prepare(
      QStringLiteral("SELECT app_name, title, %1, %2 FROM activity "
                     "WHERE %3 ORDER BY started_on DESC%4")
          .arg(kClippedStart, kClippedEnd, kOverlaps,
               limit >= 0 ? QStringLiteral(" LIMIT :limit") : QString()));
  query.bindValue(QStringLiteral(":from"), toMs(from));
  query.bindValue(QStringLiteral(":to"), toMs(to));
  if (limit >= 0) {
    query.bindValue(QStringLiteral(":limit"), limit);
  }

  if (!query.exec()) {
    qCWarning(lcRepo) << "sessions() failed:" << query.lastError().text();
    return result;
  }

  while (query.next()) {
    Activity activity;
    activity.appName = query.value(0).toString();
    activity.title = query.value(1).toString();
    activity.startedOn = fromMs(query.value(2).toLongLong());
    activity.endedOn = fromMs(query.value(3).toLongLong());
    result.append(activity);
  }
  return result;
}

QList<AppTotal> SqliteActivityRepository::appTotals(const QDateTime &from,
                                                    const QDateTime &to) const {
  QList<AppTotal> result;
  if (!_db.isOpen() || !from.isValid() || !to.isValid() || from >= to) {
    return result;
  }

  QSqlQuery query(_db);
  query.prepare(
      QStringLiteral("SELECT app_name, sum(%1 - %2) AS ms, count(*) "
                     "FROM activity WHERE %3 "
                     "GROUP BY app_name ORDER BY ms DESC, app_name ASC")
          .arg(kClippedEnd, kClippedStart, kOverlaps));
  query.bindValue(QStringLiteral(":from"), toMs(from));
  query.bindValue(QStringLiteral(":to"), toMs(to));

  if (!query.exec()) {
    qCWarning(lcRepo) << "appTotals() failed:" << query.lastError().text();
    return result;
  }

  while (query.next()) {
    AppTotal total;
    total.appName = query.value(0).toString();
    total.seconds = query.value(1).toLongLong() / 1000;
    total.sessionCount = query.value(2).toInt();
    result.append(total);
  }
  return result;
}

QList<TitleTotal>
SqliteActivityRepository::titleTotals(const QDateTime &from,
                                      const QDateTime &to) const {
  QList<TitleTotal> result;
  if (!_db.isOpen() || !from.isValid() || !to.isValid() || from >= to) {
    return result;
  }

  QSqlQuery query(_db);
  query.prepare(QStringLiteral("SELECT app_name, title, sum(%1 - %2), count(*) "
                               "FROM activity WHERE %3 "
                               "GROUP BY app_name, title")
                    .arg(kClippedEnd, kClippedStart, kOverlaps));
  query.bindValue(QStringLiteral(":from"), toMs(from));
  query.bindValue(QStringLiteral(":to"), toMs(to));

  if (!query.exec()) {
    qCWarning(lcRepo) << "titleTotals() failed:" << query.lastError().text();
    return result;
  }

  while (query.next()) {
    TitleTotal total;
    total.appName = query.value(0).toString();
    total.title = query.value(1).toString();
    total.milliseconds = query.value(2).toLongLong();
    total.sessionCount = query.value(3).toInt();
    result.append(total);
  }
  return result;
}

QList<IActivityRepository::Interval>
SqliteActivityRepository::intervals(const QDateTime &from,
                                    const QDateTime &to) const {
  QList<Interval> result;
  if (!_db.isOpen() || !from.isValid() || !to.isValid() || from >= to) {
    return result;
  }

  QSqlQuery query(_db);
  query.prepare(QStringLiteral("SELECT %1, %2 FROM activity WHERE %3 "
                               "ORDER BY started_on ASC")
                    .arg(kClippedStart, kClippedEnd, kOverlaps));
  query.bindValue(QStringLiteral(":from"), toMs(from));
  query.bindValue(QStringLiteral(":to"), toMs(to));

  if (!query.exec()) {
    qCWarning(lcRepo) << "intervals() failed:" << query.lastError().text();
    return result;
  }

  while (query.next()) {
    result.append({query.value(0).toLongLong(), query.value(1).toLongLong()});
  }
  return result;
}

RangeStats SqliteActivityRepository::stats(const QDateTime &from,
                                           const QDateTime &to) const {
  RangeStats stats;
  if (!_db.isOpen() || !from.isValid() || !to.isValid() || from >= to) {
    return stats;
  }

  QSqlQuery query(_db);
  query.prepare(
      QStringLiteral("SELECT count(*), count(DISTINCT app_name), "
                     "       sum(%2 - %1), max(%2 - %1), min(%1), max(%2) "
                     "FROM activity WHERE %3")
          .arg(kClippedStart, kClippedEnd, kOverlaps));
  query.bindValue(QStringLiteral(":from"), toMs(from));
  query.bindValue(QStringLiteral(":to"), toMs(to));

  if (!query.exec() || !query.next()) {
    qCWarning(lcRepo) << "stats() failed:" << query.lastError().text();
    return stats;
  }

  stats.sessionCount = query.value(0).toInt();
  stats.appCount = query.value(1).toInt();
  stats.totalSeconds = query.value(2).toLongLong() / 1000;
  stats.longestSessionSeconds = query.value(3).toLongLong() / 1000;
  if (!query.value(4).isNull()) {
    stats.firstActivity = fromMs(query.value(4).toLongLong());
    stats.lastActivity = fromMs(query.value(5).toLongLong());
  }

  if (stats.sessionCount > 0) {
    QSqlQuery longest(_db);
    longest.prepare(QStringLiteral("SELECT app_name FROM activity WHERE %3 "
                                   "ORDER BY (%2 - %1) DESC LIMIT 1")
                        .arg(kClippedStart, kClippedEnd, kOverlaps));
    longest.bindValue(QStringLiteral(":from"), toMs(from));
    longest.bindValue(QStringLiteral(":to"), toMs(to));
    if (longest.exec() && longest.next()) {
      stats.longestSessionApp = longest.value(0).toString();
    }
  }

  return stats;
}

QPair<QDateTime, QDateTime> SqliteActivityRepository::bounds() const {
  if (!_db.isOpen()) {
    return {};
  }

  QSqlQuery query(_db);
  if (!query.exec(QStringLiteral(
          "SELECT min(started_on), max(ended_on) FROM activity")) ||
      !query.next() || query.value(0).isNull()) {
    return {};
  }
  return {fromMs(query.value(0).toLongLong()),
          fromMs(query.value(1).toLongLong())};
}

QStringList SqliteActivityRepository::rankedAppNames(int limit) const {
  QStringList result;
  if (!_db.isOpen() || limit <= 0) {
    return result;
  }

  QSqlQuery query(_db);
  query.prepare(QStringLiteral(
      "SELECT app_name, sum(ended_on - started_on) AS ms FROM activity "
      "GROUP BY app_name ORDER BY ms DESC, app_name ASC LIMIT :limit"));
  query.bindValue(QStringLiteral(":limit"), limit);

  if (!query.exec()) {
    qCWarning(lcRepo) << "rankedAppNames() failed:" << query.lastError().text();
    return result;
  }

  while (query.next()) {
    result.append(query.value(0).toString());
  }
  return result;
}

} // namespace chronexa::activity
