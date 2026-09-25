// The log's session editor, driven through the real QML over the real stack:
// SqliteActivityRepository in a temporary file, ActivityService,
// ActivityQueryService and both controllers. Only the OS is left out -- the
// provider is a mock -- so this runs headless under `bootstrap.py test`.
//
// Set CHRONEXA_UI_SHOTS to a directory to get a screenshot of every step.

#include "application/activity/ActivityQueryService.hpp"
#include "application/activity/ActivityService.hpp"
#include "fakes/Fakes.hpp"
#include "infrastructure/activity/SqliteActivityRepository.hpp"
#include "ui/activity/ActivityQueryController.hpp"
#include "ui/activity/UserActivityController.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QQmlAbstractUrlInterceptor>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QQuickWindow>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

#include <functional>
#include <memory>

using namespace chronexa::activity;
using namespace chronexa::activity::testing;

namespace {

const QString kChrome = QStringLiteral("Google Chrome");
const QString kVideo = QStringLiteral("YouTube - lecture on CMake");

QDateTime today(int hour, int minute) {
  return QDateTime(QDate::currentDate(), QTime(hour, minute));
}

// What the tracker leaves behind: one row per minute, cut by the flushes.
QList<Activity> chain(const QString &app, const QString &title,
                      const QDateTime &from, int minutes) {
  QList<Activity> rows;
  for (int i = 0; i < minutes; ++i) {
    rows.append(Activity{app, title, from.addSecs(i * 60),
                         from.addSecs((i + 1) * 60)});
  }
  return rows;
}

// The module's qmldir prefers the copies compiled into Chronexa.exe, which a
// test executable does not have; serve the sources instead.
class SourceModule : public QQmlAbstractUrlInterceptor {
public:
  QUrl intercept(const QUrl &url, DataType) override {
    const QString prefix = QStringLiteral("/qt/qml/Chronexa/");
    if (url.scheme() != QStringLiteral("qrc") ||
        !url.path().startsWith(prefix)) {
      return url;
    }
    const QString file = url.path().mid(prefix.size());
    // The qmldir is generated; everything else is served from the sources.
    return QUrl::fromLocalFile(
        file == QStringLiteral("qmldir")
            ? QStringLiteral(CHRONEXA_QML_IMPORT_DIR "/Chronexa/qmldir")
            : QStringLiteral(CHRONEXA_QML_SOURCE_DIR "/") + file);
  }
};

QQuickItem *findItem(QQuickItem *root,
                     const std::function<bool(QQuickItem *)> &matches) {
  if (root == nullptr) {
    return nullptr;
  }
  if (matches(root)) {
    return root;
  }
  const QList<QQuickItem *> children = root->childItems();
  for (QQuickItem *child : children) {
    if (QQuickItem *found = findItem(child, matches)) {
      return found;
    }
  }
  return nullptr;
}

QQuickItem *byName(QQuickItem *root, const QString &name) {
  return findItem(root, [&](QQuickItem *item) {
    return item->objectName() == name && item->isVisible();
  });
}

// A visible button whose label is exactly `text`.
QQuickItem *button(QQuickItem *root, const QString &text) {
  return findItem(root, [&](QQuickItem *item) {
    return item->inherits("QQuickAbstractButton") && item->isVisible() &&
           item->property("text").toString() == text;
  });
}

QQuickItem *textField(QQuickItem *root) {
  return findItem(root, [](QQuickItem *item) {
    return item->inherits("QQuickTextField");
  });
}

} // namespace

class TestSessionEditing : public QObject {
  Q_OBJECT

private slots:
  void initTestCase();
  void init();
  void cleanup();

  void logShowsSessionsNotFlushCuts();
  void renameAndCategorize();
  void removeTheTimeAfterLeaving();
  void deleteASession();
  void excludedNameIsRefused();

private:
  QQuickWindow *window() const;
  QQuickItem *sessionList() const;
  QQuickItem *editor() const;
  void shot(const QString &name) const;
  void click(QQuickItem *item) const;
  void typeInto(QQuickItem *box, const QString &text) const;
  void openSession(int row) const;
  QList<Activity> stored() const;

  std::unique_ptr<QTemporaryDir> _dir;
  std::unique_ptr<SqliteActivityRepository> _repository;
  std::unique_ptr<ActivityQueryService> _queries;
  std::unique_ptr<ActivityService> _service;
  std::unique_ptr<UserActivityController> _controller;
  std::unique_ptr<ActivityQueryController> _log;
  std::unique_ptr<QQmlPropertyMap> _settings;
  SourceModule _sourceModule;
  std::unique_ptr<QQmlApplicationEngine> _engine;
};

void TestSessionEditing::initTestCase() {
  // Theme keeps the light/dark choice in QSettings: never the real one.
  QCoreApplication::setOrganizationName(QStringLiteral("ChronexaTests"));
  QCoreApplication::setApplicationName(QStringLiteral("tst_SessionEditing"));
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                     QDir::tempPath());
  QQuickWindow::setGraphicsApi(QSGRendererInterface::Software);
}

void TestSessionEditing::init() {
  _dir = std::make_unique<QTemporaryDir>();
  QVERIFY(_dir->isValid());
  _repository = std::make_unique<SqliteActivityRepository>(
      _dir->filePath(QStringLiteral("chronexa.db")));
  QVERIFY(_repository->open());

  QList<Activity> rows =
      chain(QStringLiteral("CLion"), QStringLiteral("main.cpp"), today(9, 0),
            30);
  rows += chain(kChrome, kVideo, today(9, 30), 90);
  rows += chain(QStringLiteral("Slack"), QStringLiteral("general"),
                today(11, 0), 10);
  QVERIFY(_repository->insertBatch(rows));

  _queries = std::make_unique<ActivityQueryService>(*_repository);
  _queries->setCategoryRules(CategoryRules({
      CategoryRule{QStringLiteral("Work"), {QStringLiteral("CLion")}, {}},
      CategoryRule{QStringLiteral("Distractions"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("YouTube")},
      CategoryRule{QStringLiteral("Learning"),
                   {QStringLiteral("Chrome")},
                   QStringLiteral("Course")},
  }));
  _service = std::make_unique<ActivityService>(
      *_repository, std::make_unique<MockActivityProvider>());
  _service->setPrivacyRules(
      {PrivacyRule{Privacy::Exclude, {QStringLiteral("KeePass")}, {}}});
  _controller = std::make_unique<UserActivityController>(_service.get(),
                                                         nullptr);
  _log = std::make_unique<ActivityQueryController>(*_queries);
  connect(_service.get(), &ActivityService::activityRecorded, _log.get(),
          &ActivityQueryController::refreshLater);

  _settings = std::make_unique<QQmlPropertyMap>();
  _settings->insert(QStringLiteral("resolvedLanguage"), QStringLiteral("en"));

  _engine = std::make_unique<QQmlApplicationEngine>();
  _engine->addUrlInterceptor(&_sourceModule);
  _engine->addImportPath(QStringLiteral(CHRONEXA_QML_IMPORT_DIR));
  QQmlContext *context = _engine->rootContext();
  context->setContextProperty("activityController", _controller.get());
  context->setContextProperty("settingsController", _settings.get());
  context->setContextProperty("logQuery", _log.get());
  _engine->loadData(R"(
      import QtQuick
      import Chronexa
      Window {
          width: 1100
          height: 760
          visible: true
          color: Theme.plane
          LogPage { anchors.fill: parent; query: logQuery }
      })");
  QVERIFY2(window() != nullptr, "the log page loads");
  QVERIFY(QTest::qWaitForWindowExposed(window()));
  QTRY_VERIFY(sessionList() != nullptr);
}

void TestSessionEditing::cleanup() {
  _engine.reset();
  _settings.reset();
  _log.reset();
  _controller.reset();
  _service.reset();
  _queries.reset();
  _repository.reset();
  _dir.reset();
}

QQuickWindow *TestSessionEditing::window() const {
  const QList<QObject *> roots = _engine->rootObjects();
  return roots.isEmpty() ? nullptr : qobject_cast<QQuickWindow *>(roots.first());
}

QQuickItem *TestSessionEditing::sessionList() const {
  return findItem(window()->contentItem(), [](QQuickItem *item) {
    return item->inherits("QQuickListView") &&
           item->property("model").value<QObject *>() != nullptr;
  });
}

// The editor's content while it is open, nullptr while it is not. A Dialog
// is no QQuickItem, so it is found in the object tree rather than the scene.
QQuickItem *TestSessionEditing::editor() const {
  QObject *popup = window()->findChild<QObject *>(QStringLiteral("sessionEditor"));
  if (popup == nullptr || !popup->property("visible").toBool()) {
    return nullptr;
  }
  return popup->property("contentItem").value<QQuickItem *>();
}

void TestSessionEditing::shot(const QString &name) const {
  const QString dir = qEnvironmentVariable("CHRONEXA_UI_SHOTS");
  if (!dir.isEmpty()) {
    QTest::qWait(50);
    window()->grabWindow().save(QDir(dir).filePath(name + ".png"));
  }
}

void TestSessionEditing::click(QQuickItem *item) const {
  QVERIFY(item != nullptr);
  const QPointF centre = item->mapToScene(
      QPointF(item->width() / 2, item->height() / 2));
  QTest::mouseClick(window(), Qt::LeftButton, {}, centre.toPoint());
}

void TestSessionEditing::typeInto(QQuickItem *box, const QString &text) const {
  QQuickItem *field = textField(box);
  QVERIFY(field != nullptr);
  click(field);
  QTRY_VERIFY(field->hasActiveFocus());
  QTest::keyClick(window(), Qt::Key_A, Qt::ControlModifier);
  for (const QChar c : text) {
    QTest::keyClick(window(), c.toLatin1());
  }
}

void TestSessionEditing::openSession(int row) const {
  QQuickItem *list = sessionList();
  QQuickItem *delegate = nullptr;
  QMetaObject::invokeMethod(list, "itemAtIndex", Q_RETURN_ARG(QQuickItem *, delegate),
                            Q_ARG(int, row));
  click(delegate);
  QTRY_VERIFY(editor() != nullptr);
}

QList<Activity> TestSessionEditing::stored() const {
  return _repository->sessions(today(0, 0), today(23, 59));
}

void TestSessionEditing::logShowsSessionsNotFlushCuts() {
  shot(QStringLiteral("0-log"));
  QVERIFY2(sessionList()->property("count").toInt() == 3,
           "130 stored rows are three sessions");
  QCOMPARE(stored().size(), 130);
}

void TestSessionEditing::renameAndCategorize() {
  openSession(1);
  shot(QStringLiteral("1-editor"));

  typeInto(byName(editor(), QStringLiteral("sessionTitleField")),
           QStringLiteral("Course: CMake deep dive"));
  click(button(editor(), QStringLiteral("Learning")));
  shot(QStringLiteral("2-edited"));
  click(byName(editor(), QStringLiteral("sessionSaveButton")));
  QTRY_VERIFY2(editor() == nullptr, "the editor closes once it is stored");

  int renamed = 0;
  for (const Activity &row : stored()) {
    if (row.appName == kChrome) {
      QCOMPARE(row.title, QStringLiteral("Course: CMake deep dive"));
      QCOMPARE(row.category,
               std::optional<QString>(QStringLiteral("Learning")));
      ++renamed;
    } else {
      QVERIFY2(!row.category.has_value(), "other sessions are left alone");
    }
  }
  QVERIFY2(renamed == 90, "every flush cut of the session, not one minute");

  QTRY_COMPARE(sessionList()->property("count").toInt(), 3);
  const QList<CategoryTotal> totals =
      _queries->categoryTotals(today(0, 0), today(23, 59));
  QCOMPARE(totals.first().category, QStringLiteral("Learning"));
  QCOMPARE(totals.first().seconds, 90 * 60);
  shot(QStringLiteral("3-renamed"));
}

void TestSessionEditing::removeTheTimeAfterLeaving() {
  // Left YouTube running at 10:30 and came back at 11:00.
  openSession(1);
  typeInto(byName(editor(), QStringLiteral("cutFromField")),
           QStringLiteral("1030"));
  QTest::keyClick(window(), Qt::Key_Return);
  shot(QStringLiteral("4-cut"));
  click(byName(editor(), QStringLiteral("sessionCutButton")));
  QTRY_VERIFY(editor() == nullptr);

  qint64 chromeSeconds = 0;
  for (const Activity &row : stored()) {
    if (row.appName == kChrome) {
      QVERIFY(row.endedOn <= today(10, 30));
      chromeSeconds += row.durationSeconds();
    }
  }
  QCOMPARE(chromeSeconds, 60 * 60);
  QTRY_COMPARE(_log->totalSeconds(), (30 + 60 + 10) * 60);
  shot(QStringLiteral("5-cut-done"));
}

void TestSessionEditing::deleteASession() {
  openSession(0);
  QQuickItem *remove = byName(editor(), QStringLiteral("sessionDeleteButton"));
  click(remove);
  QVERIFY2(editor() != nullptr && stored().size() == 130,
           "the first click only asks for confirmation");
  shot(QStringLiteral("6-confirm"));
  click(remove);
  QTRY_VERIFY(editor() == nullptr);

  for (const Activity &row : stored()) {
    QVERIFY(row.appName != QStringLiteral("Slack"));
  }
  QTRY_COMPARE(sessionList()->property("count").toInt(), 2);
}

void TestSessionEditing::excludedNameIsRefused() {
  openSession(2);
  typeInto(byName(editor(), QStringLiteral("sessionAppField")),
           QStringLiteral("KeePassXC"));
  click(byName(editor(), QStringLiteral("sessionSaveButton")));

  QQuickItem *error = byName(editor(), QStringLiteral("sessionError"));
  QVERIFY2(error != nullptr, "the reason is shown, the editor stays open");
  QVERIFY(error->property("text").toString().contains(
      QStringLiteral("privacy")));
  shot(QStringLiteral("7-refused"));
  for (const Activity &row : stored()) {
    QVERIFY(!row.appName.contains(QStringLiteral("KeePass")));
  }
}

QTEST_MAIN(TestSessionEditing)
#include "tst_SessionEditing.moc"
