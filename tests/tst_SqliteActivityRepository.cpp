#include "fakes/Fakes.hpp"
#include "infrastructure/activity/SqliteActivityRepository.hpp"

#include <QFile>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

#include <algorithm>
#include <memory>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const TitleTotal *find(const QList<TitleTotal> &totals, const QString &app,
                       const QString &title) {
  const auto it = std::find_if(totals.cbegin(), totals.cend(),
                         [&](const TitleTotal &total) {
                           return total.appName == app && total.title == title;
                         });
  return it == totals.cend() ? nullptr : &*it;
}

// Whether the text is anywhere in the database or its WAL, in any encoding
// SQLite might have stored it in.
bool fileContains(const QString &databasePath, const QString &text) {
  for (const QString &path : {databasePath, databasePath + "-wal"}) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
      continue;
    }
    const QByteArray bytes = file.readAll();
    const QByteArray utf16(reinterpret_cast<const char *>(text.utf16()),
                           text.size() * 2);
    if (bytes.contains(text.toUtf8()) || bytes.contains(utf16)) {
      return true;
    }
  }
  return false;
}

// A session as the tracker leaves it: one window, cut at every minute.
QList<Activity> chain(const QString &app, const QString &title, QTime from,
                      int minutes) {
  QList<Activity> rows;
  for (int i = 0; i < minutes; ++i) {
    const QDateTime start = utc(from.hour(), from.minute()).addSecs(i * 60);
    rows.append(Activity{app, title, start, start.addSecs(60)});
  }
  return rows;
}

} // namespace

class TestSqliteActivityRepository : public QObject {
  Q_OBJECT

private slots:
  void init();
  void cleanup();

  void titleTotalsGroupsByAppAndTitle();
  void titleTotalsClipsToRange();
  void titleTotalsRejectsEmptyRange();

  void titleLessSessionIsStored();
  void windowsAreDistinct();
  void redactRemovesAndHides();
  void redactOfNothingChangesNothing();
  void redactLeavesNoTraceInTheFile();
  void clearLeavesNoTraceInTheFile();

  void categoryIsStoredAsGiven();
  void editRewritesTheWholeSession();
  void editCanHandCategoryBackToTheRules();
  void editOfNothingChangesNothing();
  void editLeavesNoTraceOfTheOldTitle();
  void cut_data();
  void cut();
  void cutLeavesOtherWindowsAlone();
  void olderFileIsMigrated();

private:
  QString databasePath() const {
    return _dir->filePath(QStringLiteral("test.db"));
  }

  QList<Activity> allRows() const;

  std::unique_ptr<QTemporaryDir> _dir;
  std::unique_ptr<SqliteActivityRepository> _repository;
};

void TestSqliteActivityRepository::init() {
  _dir = std::make_unique<QTemporaryDir>();
  QVERIFY(_dir->isValid());
  _repository = std::make_unique<SqliteActivityRepository>(
      _dir->filePath(QStringLiteral("test.db")));
  QVERIFY(_repository->open());
}

void TestSqliteActivityRepository::cleanup() {
  _repository.reset();
  _dir.reset();
}

void TestSqliteActivityRepository::titleTotalsGroupsByAppAndTitle() {
  const QString chrome = QStringLiteral("Google Chrome");
  QVERIFY(_repository->insertBatch({
      Activity{chrome, QStringLiteral("YouTube"), utc(10, 0), utc(10, 5)},
      Activity{chrome, QStringLiteral("PR #1"), utc(10, 5), utc(10, 20)},
      Activity{chrome, QStringLiteral("YouTube"), utc(11, 0), utc(11, 1)},
      Activity{QStringLiteral("Slack"), QStringLiteral("YouTube"), utc(12, 0),
               utc(12, 2)},
  }));

  const QList<TitleTotal> totals =
      _repository->titleTotals(utc(0, 0), utc(23, 0));
  QCOMPARE(totals.size(), 3);

  const TitleTotal *youtube = find(totals, chrome, QStringLiteral("YouTube"));
  QVERIFY(youtube);
  QCOMPARE(youtube->milliseconds, 6 * 60 * 1000);
  QCOMPARE(youtube->sessionCount, 2);

  const TitleTotal *pr = find(totals, chrome, QStringLiteral("PR #1"));
  QVERIFY(pr);
  QCOMPARE(pr->milliseconds, 15 * 60 * 1000);

  const TitleTotal *slack =
      find(totals, QStringLiteral("Slack"), QStringLiteral("YouTube"));
  QVERIFY2(slack, "the same title under another app is its own group");
  QCOMPARE(slack->milliseconds, 2 * 60 * 1000);
}

void TestSqliteActivityRepository::titleTotalsClipsToRange() {
  QVERIFY(_repository->insertBatch({
      Activity{QStringLiteral("CLion"), QStringLiteral("a.cpp"), utc(9, 50),
               utc(10, 10)},
      Activity{QStringLiteral("CLion"), QStringLiteral("a.cpp"), utc(11, 0),
               utc(11, 30)},
      Activity{QStringLiteral("CLion"), QStringLiteral("old.cpp"), utc(8, 0),
               utc(9, 0)},
  }));

  const QList<TitleTotal> totals =
      _repository->titleTotals(utc(10, 0), utc(11, 15));
  QCOMPARE(totals.size(), 1);
  QCOMPARE(totals.first().title, QStringLiteral("a.cpp"));
  QVERIFY2(totals.first().milliseconds == 25 * 60 * 1000,
           "10 min after 10:00 plus 15 min before 11:15");
  QCOMPARE(totals.first().sessionCount, 2);
}

void TestSqliteActivityRepository::titleTotalsRejectsEmptyRange() {
  QVERIFY(_repository->insertBatch({Activity{
      QStringLiteral("CLion"), QStringLiteral("a"), utc(10, 0), utc(11, 0)}}));
  QVERIFY(_repository->titleTotals(utc(11, 0), utc(10, 0)).isEmpty());
  QVERIFY(_repository->titleTotals(QDateTime(), utc(10, 0)).isEmpty());
}

// A hidden title arrives as a null QString, which binds as SQL NULL; the
// NOT NULL column must not turn that into a lost session.
void TestSqliteActivityRepository::titleLessSessionIsStored() {
  QVERIFY(_repository->insertBatch({Activity{
      QStringLiteral("Telegram"), QString(), utc(10, 0), utc(10, 5)}}));

  const QList<Activity> sessions = _repository->sessions(utc(0, 0), utc(23, 0));
  QCOMPARE(sessions.size(), 1);
  QCOMPARE(sessions.first().appName, QStringLiteral("Telegram"));
  QVERIFY(sessions.first().title.isEmpty());

  QVERIFY2(_repository->redact({WindowRef{QStringLiteral("Telegram"), {}}},
                               {}) == 1,
           "a title-less window can be removed by its null title too");
  QVERIFY(_repository->sessions(utc(0, 0), utc(23, 0)).isEmpty());
}

void TestSqliteActivityRepository::windowsAreDistinct() {
  QVERIFY(_repository->insertBatch({
      Activity{QStringLiteral("Slack"), QStringLiteral("general"), utc(10, 0),
               utc(10, 5)},
      Activity{QStringLiteral("Slack"), QStringLiteral("general"), utc(11, 0),
               utc(11, 5)},
      Activity{QStringLiteral("Slack"), QStringLiteral("random"), utc(12, 0),
               utc(12, 5)},
  }));

  const std::optional<QList<WindowRef>> read = _repository->windows();
  QVERIFY(read.has_value());
  const QList<WindowRef> &windows = *read;
  QCOMPARE(windows.size(), 2);
  QVERIFY(windows.contains(
      WindowRef{QStringLiteral("Slack"), QStringLiteral("general")}));
  QVERIFY(windows.contains(
      WindowRef{QStringLiteral("Slack"), QStringLiteral("random")}));
}

void TestSqliteActivityRepository::redactRemovesAndHides() {
  const QString telegram = QStringLiteral("Telegram Desktop");
  QVERIFY(_repository->insertBatch({
      Activity{QStringLiteral("KeePassXC"), QStringLiteral("bank.kdbx"),
               utc(9, 0), utc(9, 5)},
      Activity{telegram, QStringLiteral("Olena"), utc(10, 0), utc(10, 5)},
      Activity{telegram, QStringLiteral("Olena"), utc(11, 0), utc(11, 5)},
      Activity{telegram, QStringLiteral("Petro"), utc(12, 0), utc(12, 5)},
      Activity{QStringLiteral("CLion"), QStringLiteral("main.cpp"), utc(13, 0),
               utc(13, 5)},
  }));

  const int changed = _repository->redact(
      {WindowRef{QStringLiteral("KeePassXC"), QStringLiteral("bank.kdbx")}},
      {WindowRef{telegram, QStringLiteral("Olena")}});
  QCOMPARE(changed, 3);

  const QList<Activity> sessions = _repository->sessions(utc(0, 0), utc(23, 0));
  QCOMPARE(sessions.size(), 4);
  QStringList titles;
  for (const Activity &session : sessions) {
    QVERIFY(session.appName != QStringLiteral("KeePassXC"));
    titles.append(session.title);
  }
  titles.sort();
  QCOMPARE(titles, (QStringList{QString(), QString(), QStringLiteral("Petro"),
                                QStringLiteral("main.cpp")}));

  const QList<TitleTotal> totals =
      _repository->titleTotals(utc(0, 0), utc(23, 0));
  const TitleTotal *hidden = find(totals, telegram, QString());
  QVERIFY2(hidden && hidden->milliseconds == 10 * 60 * 1000,
           "hiding a title keeps the time");
}

void TestSqliteActivityRepository::redactOfNothingChangesNothing() {
  QVERIFY(_repository->insertBatch({Activity{
      QStringLiteral("CLion"), QStringLiteral("a"), utc(10, 0), utc(10, 5)}}));
  QCOMPARE(_repository->redact({}, {}), 0);
  QCOMPARE(_repository->redact(
               {WindowRef{QStringLiteral("Nope"), QStringLiteral("a")}},
               {WindowRef{QStringLiteral("CLion"), QString()}}),
           0);
  QCOMPARE(_repository->sessions(utc(0, 0), utc(23, 0)).size(), 1);
}

void TestSqliteActivityRepository::redactLeavesNoTraceInTheFile() {
  const QString secret = QStringLiteral("Olena: the door code is 4411");
  const QString removedSecret = QStringLiteral("Passwords of Aunt Mariia");
  QList<Activity> batch;
  // Enough ordinary rows around the secret that its page stays in use.
  for (int i = 0; i < 50; ++i) {
    batch.append(Activity{QStringLiteral("CLion"),
                          QStringLiteral("file%1.cpp").arg(i), utc(8, i),
                          utc(8, i + 1)});
  }
  batch.append(Activity{QStringLiteral("Telegram"), secret, utc(10, 0),
                        utc(10, 5)});
  batch.append(Activity{QStringLiteral("KeePassXC"), removedSecret, utc(11, 0),
                        utc(11, 5)});
  QVERIFY(_repository->insertBatch(batch));
  QVERIFY2(fileContains(databasePath(), secret) &&
               fileContains(databasePath(), removedSecret),
           "the probe finds the text before the redaction");

  QCOMPARE(
      _repository->redact({WindowRef{QStringLiteral("KeePassXC"), removedSecret}},
                          {WindowRef{QStringLiteral("Telegram"), secret}}),
      2);

  QVERIFY2(!fileContains(databasePath(), secret),
           "the hidden title is gone from the file, not just the query");
  QVERIFY2(!fileContains(databasePath(), removedSecret),
           "the removed session is gone from the file");
  QVERIFY(fileContains(databasePath(), QStringLiteral("file7.cpp")));
}

void TestSqliteActivityRepository::clearLeavesNoTraceInTheFile() {
  const QString secret = QStringLiteral("Olena: the door code is 4411");
  QVERIFY(_repository->insertBatch(
      {Activity{QStringLiteral("Telegram"), secret, utc(10, 0), utc(10, 5)}}));
  QVERIFY(fileContains(databasePath(), secret));

  QVERIFY(_repository->clearAll());
  QVERIFY(!fileContains(databasePath(), secret));
}

// Every stored row of the day, oldest first.
QList<Activity> TestSqliteActivityRepository::allRows() const {
  QList<Activity> rows = _repository->sessions(utc(0, 0), utc(23, 59));
  std::reverse(rows.begin(), rows.end());
  return rows;
}

void TestSqliteActivityRepository::categoryIsStoredAsGiven() {
  Activity unset{QStringLiteral("CLion"), QStringLiteral("a"), utc(10, 0),
                 utc(10, 5)};
  Activity none = unset;
  none.startedOn = utc(11, 0);
  none.endedOn = utc(11, 5);
  none.category = QStringLiteral("");
  Activity work = unset;
  work.startedOn = utc(12, 0);
  work.endedOn = utc(12, 5);
  work.category = QStringLiteral("Work");
  QVERIFY(_repository->insertBatch({unset, none, work}));

  const QList<Activity> rows = allRows();
  QCOMPARE(rows.size(), 3);
  QVERIFY2(!rows.at(0).category.has_value(), "unset reads back as unset");
  QVERIFY2(rows.at(1).category == std::optional<QString>(QStringLiteral("")),
           "'no category' must not collapse into 'the rules decide'");
  QCOMPARE(rows.at(2).category, std::optional<QString>(QStringLiteral("Work")));

  const QList<TitleTotal> totals =
      _repository->titleTotals(utc(0, 0), utc(23, 0));
  QVERIFY2(totals.size() == 3, "one window, split by its three categories");
}

void TestSqliteActivityRepository::editRewritesTheWholeSession() {
  const QString chrome = QStringLiteral("Google Chrome");
  const QString youtube = QStringLiteral("YouTube");
  QList<Activity> rows = chain(chrome, youtube, QTime(9, 0), 2);
  rows += chain(chrome, youtube, QTime(10, 0), 3);
  rows += chain(QStringLiteral("CLion"), youtube, QTime(10, 3), 1);
  QVERIFY(_repository->insertBatch(rows));

  // The session the log shows for 10:00-10:03.
  const int changed = _repository->editSessions(
      {chrome, youtube}, utc(10, 0), utc(10, 3),
      SessionEdit{chrome, QStringLiteral("Course: CMake"),
                  QStringLiteral("Learning")});
  QVERIFY2(changed == 3, "every row the flushes cut the session into");

  const QList<Activity> after = allRows();
  QCOMPARE(after.size(), 6);
  for (int i = 0; i < 2; ++i) {
    QVERIFY2(after.at(i).title == youtube && !after.at(i).category,
             "the earlier session in the same window is left alone");
  }
  for (int i = 2; i < 5; ++i) {
    QCOMPARE(after.at(i).title, QStringLiteral("Course: CMake"));
    QCOMPARE(after.at(i).category,
             std::optional<QString>(QStringLiteral("Learning")));
  }
  QVERIFY2(after.at(5).appName == QStringLiteral("CLion") &&
               after.at(5).title == youtube,
           "the next session, touching it, is another window");
}

void TestSqliteActivityRepository::editCanHandCategoryBackToTheRules() {
  Activity row{QStringLiteral("CLion"), QStringLiteral("a"), utc(10, 0),
               utc(10, 5), QStringLiteral("Fun")};
  QVERIFY(_repository->insertBatch({row}));

  QCOMPARE(_repository->editSessions(
               {row.appName, row.title}, row.startedOn, row.endedOn,
               SessionEdit{QStringLiteral("CLion (GUI launcher)"),
                           QStringLiteral(""), std::nullopt}),
           1);

  const QList<Activity> after = allRows();
  QCOMPARE(after.size(), 1);
  QCOMPARE(after.first().appName, QStringLiteral("CLion (GUI launcher)"));
  QVERIFY(after.first().title.isEmpty());
  QVERIFY(!after.first().category.has_value());
}

void TestSqliteActivityRepository::editOfNothingChangesNothing() {
  QVERIFY(_repository->insertBatch({Activity{
      QStringLiteral("CLion"), QStringLiteral("a"), utc(10, 0), utc(10, 5)}}));

  QCOMPARE(_repository->editSessions(
               {QStringLiteral("CLion"), QStringLiteral("b")}, utc(10, 0),
               utc(10, 5), SessionEdit{QStringLiteral("X"), {}, std::nullopt}),
           0);
  QCOMPARE(_repository->editSessions(
               {QStringLiteral("CLion"), QStringLiteral("a")}, utc(10, 5),
               utc(11, 0), SessionEdit{QStringLiteral("X"), {}, std::nullopt}),
           0);
  QVERIFY2(_repository->editSessions(
               {QStringLiteral("CLion"), QStringLiteral("a")}, utc(10, 0),
               utc(10, 5), SessionEdit{QString(), {}, std::nullopt}) == -1,
           "a session without an application is refused");
  QCOMPARE(allRows().first().appName, QStringLiteral("CLion"));
}

void TestSqliteActivityRepository::editLeavesNoTraceOfTheOldTitle() {
  const QString secret = QStringLiteral("Olena: the door code is 4411");
  QList<Activity> batch;
  for (int i = 0; i < 50; ++i) {
    batch.append(Activity{QStringLiteral("CLion"),
                          QStringLiteral("file%1.cpp").arg(i), utc(8, i),
                          utc(8, i + 1)});
  }
  batch += chain(QStringLiteral("Telegram"), secret, QTime(10, 0), 3);
  QVERIFY(_repository->insertBatch(batch));
  QVERIFY(fileContains(databasePath(), secret));

  QCOMPARE(_repository->editSessions(
               {QStringLiteral("Telegram"), secret}, utc(10, 0), utc(10, 3),
               SessionEdit{QStringLiteral("Telegram"), QStringLiteral("Chat"),
                           std::nullopt}),
           3);
  QVERIFY2(!fileContains(databasePath(), secret),
           "a renamed title is gone from the file, not just the query");
}

void TestSqliteActivityRepository::cut_data() {
  QTest::addColumn<QTime>("from");
  QTest::addColumn<QTime>("to");
  // What is left of the 10:00-10:03 session, as start-end minute pairs.
  QTest::addColumn<QList<int>>("left");

  QTest::newRow("whole session") << QTime(10, 0) << QTime(10, 3)
                                 << QList<int>{};
  QTest::newRow("forgot to stop") << QTime(10, 1, 30) << QTime(10, 3)
                                  << QList<int>{0, 60, 60, 90};
  QTest::newRow("late start") << QTime(10, 0) << QTime(10, 0, 30)
                              << QList<int>{30, 60, 60, 120, 120, 180};
  QTest::newRow("middle of one row")
      << QTime(10, 1, 20) << QTime(10, 1, 40)
      << QList<int>{0, 60, 60, 80, 100, 120, 120, 180};
  QTest::newRow("across rows") << QTime(10, 0, 30) << QTime(10, 2, 30)
                               << QList<int>{0, 30, 150, 180};
}

void TestSqliteActivityRepository::cut() {
  QFETCH(QTime, from);
  QFETCH(QTime, to);
  QFETCH(QList<int>, left);

  const QString app = QStringLiteral("Google Chrome");
  const QString title = QStringLiteral("YouTube");
  QList<Activity> rows = chain(app, title, QTime(10, 0), 3);
  for (Activity &row : rows) {
    row.category = QStringLiteral("Fun");
  }
  // Neighbours that must survive: the same window before, another one after.
  rows.prepend(Activity{app, title, utc(9, 0), utc(9, 5)});
  rows.append(Activity{QStringLiteral("CLion"), title, utc(10, 3), utc(10, 4)});
  QVERIFY(_repository->insertBatch(rows));

  const QDateTime base = utc(10, 0);
  const QDateTime cutFrom(base.date(), from, QTimeZone::UTC);
  const QDateTime cutTo(base.date(), to, QTimeZone::UTC);
  QVERIFY(_repository->cutSessions({app, title}, cutFrom, cutTo) > 0);

  const QList<Activity> after = allRows();
  QCOMPARE(after.size(), 2 + left.size() / 2);
  QCOMPARE(after.first().startedOn, utc(9, 0));
  QCOMPARE(after.last().appName, QStringLiteral("CLion"));
  for (int i = 0; i < left.size() / 2; ++i) {
    const Activity &row = after.at(i + 1);
    QCOMPARE(base.secsTo(row.startedOn), left.at(i * 2));
    QCOMPARE(base.secsTo(row.endedOn), left.at(i * 2 + 1));
    QVERIFY2(row.category == std::optional<QString>(QStringLiteral("Fun")),
             "a split keeps the category on both halves");
  }

  const RangeStats stats = _repository->stats(utc(10, 0), utc(10, 3));
  QCOMPARE(stats.totalSeconds, 180 - cutFrom.secsTo(cutTo));
}

void TestSqliteActivityRepository::cutLeavesOtherWindowsAlone() {
  QVERIFY(_repository->insertBatch({
      Activity{QStringLiteral("CLion"), QStringLiteral("a"), utc(10, 0),
               utc(10, 5)},
      Activity{QStringLiteral("CLion"), QStringLiteral("b"), utc(10, 5),
               utc(10, 10)},
  }));

  QCOMPARE(_repository->cutSessions(
               {QStringLiteral("CLion"), QStringLiteral("b")}, utc(10, 0),
               utc(10, 5)),
           0);
  QCOMPARE(_repository->cutSessions(
               {QStringLiteral("CLion"), QStringLiteral("a")}, utc(10, 5),
               utc(10, 0)),
           -1);
  QCOMPARE(allRows().size(), 2);
}

// A chronexa.db from before the category column: it has to open, keep its
// history, and take categories from then on.
void TestSqliteActivityRepository::olderFileIsMigrated() {
  _repository.reset();
  const QString path = _dir->filePath(QStringLiteral("old.db"));
  {
    QSqlDatabase db =
        QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), "old");
    db.setDatabaseName(path);
    QVERIFY(db.open());
    QSqlQuery query(db);
    QVERIFY(query.exec(QStringLiteral(
        "CREATE TABLE activity (id INTEGER PRIMARY KEY, app_name TEXT NOT "
        "NULL, title TEXT NOT NULL, started_on INTEGER NOT NULL, ended_on "
        "INTEGER NOT NULL)")));
    QVERIFY(query.exec(
        QStringLiteral("INSERT INTO activity (app_name, title, started_on, "
                       "ended_on) VALUES ('CLion', 'a', %1, %2)")
            .arg(utc(10, 0).toMSecsSinceEpoch())
            .arg(utc(10, 5).toMSecsSinceEpoch())));
    db.close();
  }
  QSqlDatabase::removeDatabase("old");

  for (int open = 0; open < 2; ++open) {
    _repository = std::make_unique<SqliteActivityRepository>(path);
    QVERIFY2(_repository->open(), "opens, and opens again once migrated");
    _repository.reset();
  }
  _repository = std::make_unique<SqliteActivityRepository>(path);
  QVERIFY(_repository->open());

  QList<Activity> rows = allRows();
  QCOMPARE(rows.size(), 1);
  QVERIFY(!rows.first().category.has_value());
  QCOMPARE(_repository->editSessions(
               {QStringLiteral("CLion"), QStringLiteral("a")}, utc(10, 0),
               utc(10, 5),
               SessionEdit{QStringLiteral("CLion"), QStringLiteral("a"),
                           QStringLiteral("Work")}),
           1);
  QCOMPARE(allRows().first().category,
           std::optional<QString>(QStringLiteral("Work")));
}

QTEST_MAIN(TestSqliteActivityRepository)
#include "tst_SqliteActivityRepository.moc"
