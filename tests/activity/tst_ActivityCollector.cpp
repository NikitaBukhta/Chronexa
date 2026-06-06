#include <QtTest>

#include "domain/activity/ActivityCollector.hpp"

using namespace chronexa::activity;

namespace {

// Fixed origin -- every scenario works with fabricated, deterministic
// timestamps relative to it.
const QDateTime kT0 =
    QDateTime::fromString(QStringLiteral("2026-01-01T10:00:00"), Qt::ISODate);

QDateTime at(const qint64 seconds) { return kT0.addSecs(seconds); }

} // namespace

class tst_ActivityCollector : public QObject {
  Q_OBJECT

private slots:
  void switchClosesPreviousActivity();
  void duplicateNotificationIsIgnored();
  void shortGlitchIsDropped();
  void sameActivityWithSmallGapIsMerged();
  void tabDomainChangeSplitsActivity();
  void drainReopensCurrentActivity();
  void resetDropsEverything();
};

void tst_ActivityCollector::switchClosesPreviousActivity() {
  ActivityCollector collector;
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(0));
  collector.notifyActivityChanged("chrome", "Chrome", "Docs", "qt.io", at(60));

  const QList<Activity> handled = collector.drainHandled(at(120));
  QCOMPARE(handled.size(), 2);
  QCOMPARE(handled[0].appId, QStringLiteral("code"));
  QCOMPARE(handled[0].startedOn, at(0));
  QCOMPARE(handled[0].endedOn, at(60));
  QCOMPARE(handled[1].appId, QStringLiteral("chrome"));
  QCOMPARE(handled[1].tabDomain, QStringLiteral("qt.io"));
  QCOMPARE(handled[1].endedOn, at(120));
}

void tst_ActivityCollector::duplicateNotificationIsIgnored() {
  ActivityCollector collector;
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(0));
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(30));

  const QList<Activity> handled = collector.drainHandled(at(60));
  QCOMPARE(handled.size(), 1);
  // The original start time survives the duplicate notification.
  QCOMPARE(handled[0].startedOn, at(0));
  QCOMPARE(handled[0].endedOn, at(60));
}

void tst_ActivityCollector::shortGlitchIsDropped() {
  ActivityCollector collector;
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(0));
  // Alt-tab glitch: foreground for less than a second.
  collector.notifyActivityChanged("explorer", "Explorer", "Desktop", {},
                                  at(10));
  collector.notifyActivityChanged("chrome", "Chrome", "Docs", {},
                                  at(10).addMSecs(500));

  const QList<Activity> handled = collector.drainHandled(at(60));
  QCOMPARE(handled.size(), 2);
  QCOMPARE(handled[0].appId, QStringLiteral("code"));
  QCOMPARE(handled[1].appId, QStringLiteral("chrome"));
}

void tst_ActivityCollector::sameActivityWithSmallGapIsMerged() {
  ActivityCollector collector;
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(0));
  // Sub-second detour dropped as a glitch, then back to the same activity:
  // the gap is below the merge threshold, so one continuous record remains.
  collector.notifyActivityChanged("explorer", "Explorer", "Desktop", {},
                                  at(20));
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {},
                                  at(20).addMSecs(800));

  const QList<Activity> handled = collector.drainHandled(at(60));
  QCOMPARE(handled.size(), 1);
  QCOMPARE(handled[0].appId, QStringLiteral("code"));
  QCOMPARE(handled[0].startedOn, at(0));
  QCOMPARE(handled[0].endedOn, at(60));
}

void tst_ActivityCollector::tabDomainChangeSplitsActivity() {
  ActivityCollector collector;
  collector.notifyActivityChanged("chrome", "Chrome", "Tab", "github.com",
                                  at(0));
  collector.notifyActivityChanged("chrome", "Chrome", "Tab", "qt.io", at(30));

  const QList<Activity> handled = collector.drainHandled(at(60));
  QCOMPARE(handled.size(), 2);
  QCOMPARE(handled[0].tabDomain, QStringLiteral("github.com"));
  QCOMPARE(handled[1].tabDomain, QStringLiteral("qt.io"));
}

void tst_ActivityCollector::drainReopensCurrentActivity() {
  ActivityCollector collector;
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(0));

  QList<Activity> first = collector.drainHandled(at(60));
  QCOMPARE(first.size(), 1);
  QCOMPARE(first[0].endedOn, at(60));

  // No new notification: the open activity continues seamlessly.
  const QList<Activity> second = collector.drainHandled(at(120));
  QCOMPARE(second.size(), 1);
  QCOMPARE(second[0].startedOn, at(60));
  QCOMPARE(second[0].endedOn, at(120));
}

void tst_ActivityCollector::resetDropsEverything() {
  ActivityCollector collector;
  collector.notifyActivityChanged("code", "VS Code", "main.cpp", {}, at(0));
  collector.reset();

  QCOMPARE(collector.drainHandled(at(60)).size(), 0);
}

QTEST_APPLESS_MAIN(tst_ActivityCollector)
#include "tst_ActivityCollector.moc"
