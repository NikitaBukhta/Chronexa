#pragma once

// Test doubles for the interfaces application/ and infrastructure/ depend on.
// Header-only: each test executable compiles only what it includes.

#include "infrastructure/activity/IActivityRepository.hpp"
#include "infrastructure/activity/IForegroundProbe.hpp"
#include "infrastructure/activity/IUserActivityProvider.hpp"
#include "infrastructure/system/INotifier.hpp"

#include <QDateTime>
#include <QList>
#include <QTimeZone>

namespace chronexa::activity::testing {

// A clock that only moves when told to.
class ManualClock {
public:
  explicit ManualClock(QDateTime start) : _now(std::move(start)) {}

  QDateTime now() const { return _now; }
  void advance(qint64 seconds) { _now = _now.addSecs(seconds); }

private:
  QDateTime _now;
};

// Returns whatever the test last put in front of the "user".
class FakeForegroundProbe : public IForegroundProbe {
public:
  ForegroundSample current;
  int sampleCount = 0;

  ForegroundSample sample() override {
    ++sampleCount;
    return current;
  }

  void show(const QString &appName, const QString &title) {
    current = {false, appName, title};
  }
  void noWindow() { current = {false, {}, {}}; }
  void goIdle() { current = {true, {}, {}}; }
};

// Records every write; serves canned title totals to the read side.
class FakeActivityRepository : public IActivityRepository {
public:
  QList<Activity> inserted;
  QList<TitleTotal> titles;
  bool failInserts = false;
  bool failRedact = false;
  int insertCalls = 0;
  int clearCalls = 0;
  QList<WindowRef> removed;
  QList<WindowRef> hidden;

  mutable int titleTotalsCalls = 0;
  mutable QDateTime lastFrom;
  mutable QDateTime lastTo;

  bool open() override { return true; }
  bool insertBatch(const QList<Activity> &activities) override {
    ++insertCalls;
    if (failInserts || activities.isEmpty()) {
      return false;
    }
    inserted += activities;
    return true;
  }
  bool clearAll() override {
    ++clearCalls;
    inserted.clear();
    return true;
  }

  bool failWindows = false;

  std::optional<QList<WindowRef>> windows() const override {
    if (failWindows) {
      return std::nullopt;
    }
    QList<WindowRef> result;
    for (const Activity &activity : inserted) {
      const WindowRef window{activity.appName, activity.title};
      if (!result.contains(window)) {
        result.append(window);
      }
    }
    return result;
  }
  // Applies the redaction to `inserted`, so a test can look at the outcome.
  int redact(const QList<WindowRef> &remove,
             const QList<WindowRef> &hideTitle) override {
    if (failRedact) {
      return -1;
    }
    removed += remove;
    hidden += hideTitle;
    int changed = 0;
    QList<Activity> kept;
    for (Activity activity : inserted) {
      const WindowRef window{activity.appName, activity.title};
      if (remove.contains(window)) {
        ++changed;
        continue;
      }
      if (hideTitle.contains(window) && !activity.title.isEmpty()) {
        activity.title.clear();
        ++changed;
      }
      kept.append(activity);
    }
    inserted = kept;
    return changed;
  }

  struct EditCall {
    WindowRef window;
    QDateTime from;
    QDateTime to;
    SessionEdit edit;
  };
  struct CutCall {
    WindowRef window;
    QDateTime from;
    QDateTime to;
  };
  QList<EditCall> edits;
  QList<CutCall> cuts;
  // What editSessions()/cutSessions() report; -1 is a failed write.
  int editResult = 1;

  int editSessions(const WindowRef &window, const QDateTime &from,
                   const QDateTime &to, const SessionEdit &edit) override {
    edits.append({window, from, to, edit});
    return editResult;
  }
  int cutSessions(const WindowRef &window, const QDateTime &from,
                  const QDateTime &to) override {
    cuts.append({window, from, to});
    return editResult;
  }

  QList<Activity> sessions(const QDateTime &, const QDateTime &,
                           int) const override {
    return inserted;
  }
  QList<AppTotal> appTotals(const QDateTime &,
                            const QDateTime &) const override {
    return {};
  }
  QList<TitleTotal> titleTotals(const QDateTime &from,
                                const QDateTime &to) const override {
    ++titleTotalsCalls;
    lastFrom = from;
    lastTo = to;
    return titles;
  }
  QList<Interval> intervals(const QDateTime &,
                            const QDateTime &) const override {
    return {};
  }
  RangeStats stats(const QDateTime &, const QDateTime &) const override {
    return {};
  }
  QPair<QDateTime, QDateTime> bounds() const override { return {}; }
  QStringList rankedAppNames(int) const override { return {}; }
};

// A provider under full test control; remembers the calls made on it.
class MockActivityProvider : public IUserActivityProvider {
public:
  QList<Activity> pendingEvents;
  SessionKey sessionKey;
  PrivacyFilter privacyFilter;
  bool running = false;
  int startCalls = 0;
  int stopCalls = 0;

  void start() override {
    ++startCalls;
    running = true;
  }
  void stop() override {
    ++stopCalls;
    running = false;
  }
  bool isRunning() const override { return running; }
  void setSessionKey(SessionKey key) override { sessionKey = std::move(key); }
  void setPrivacyFilter(PrivacyFilter filter) override {
    privacyFilter = std::move(filter);
  }
  QList<Activity> drainEvents() override {
    QList<Activity> drained;
    drained.swap(pendingEvents);
    return drained;
  }
  std::optional<Activity> currentSession() const override {
    return std::nullopt;
  }
  bool isIdle() const override { return false; }
};

inline QDateTime utc(int hour, int minute, int second = 0) {
  return {QDate(2026, 9, 1), QTime(hour, minute, second),
                   QTimeZone::UTC};
}

class FakeNotifier : public chronexa::system::INotifier {
public:
  struct Message {
    QString title;
    QString text;
  };
  QList<Message> messages;
  bool supported = true;

  bool isSupported() const override { return supported; }
  void notify(const QString &title, const QString &message) override {
    messages.append({title, message});
  }
};

} // namespace chronexa::activity::testing
