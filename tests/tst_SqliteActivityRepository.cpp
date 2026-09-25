#include "fakes/Fakes.hpp"
#include "infrastructure/activity/SqliteActivityRepository.hpp"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

#include <algorithm>
#include <memory>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const TitleTotal *find(const QList<TitleTotal> &totals, const QString &app,
                       const QString &title) {
  auto it = std::find_if(totals.cbegin(), totals.cend(),
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

private:
  QString databasePath() const {
    return _dir->filePath(QStringLiteral("test.db"));
  }

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

QTEST_MAIN(TestSqliteActivityRepository)
#include "tst_SqliteActivityRepository.moc"
