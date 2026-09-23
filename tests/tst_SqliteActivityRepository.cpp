#include "infrastructure/activity/SqliteActivityRepository.hpp"

#include <QTemporaryDir>
#include <QTest>
#include <QTimeZone>

#include <algorithm>
#include <memory>

using namespace chronexa::activity;

namespace {

QDateTime utc(int hour, int minute, int second = 0) {
  return QDateTime(QDate(2026, 9, 1), QTime(hour, minute, second),
                   QTimeZone::UTC);
}

const TitleTotal *find(const QList<TitleTotal> &totals, const QString &app,
                       const QString &title) {
  auto it = std::find_if(totals.cbegin(), totals.cend(),
                         [&](const TitleTotal &total) {
                           return total.appName == app && total.title == title;
                         });
  return it == totals.cend() ? nullptr : &*it;
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

private:
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

QTEST_MAIN(TestSqliteActivityRepository)
#include "tst_SqliteActivityRepository.moc"
