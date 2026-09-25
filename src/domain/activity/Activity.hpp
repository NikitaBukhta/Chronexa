#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

#include <optional>

namespace chronexa::activity {

struct Activity {
  QString appName;
  QString title;
  QDateTime startedOn;
  QDateTime endedOn;
  std::optional<QString> category;

  [[nodiscard]] bool isValid() const;
  [[nodiscard]] qint64 durationSeconds() const;
};

struct WindowRef {
  QString appName;
  QString title;

  bool operator==(const WindowRef &other) const {
    return appName == other.appName && title == other.title;
  }
};

// A correction made in the log to what the tracker recorded.
struct SessionEdit {
  QString appName;
  QString title;
  std::optional<QString> category;

  // Trimmed; a category of only blanks means "no category".
  [[nodiscard]] SessionEdit normalized() const;
  // A session without an application cannot be stored.
  [[nodiscard]] bool isValid() const;
};

// Largest gap between two stored rows that still reads as one session.
constexpr qint64 kJoinGapMs = 1000;

// Every flush -- once a minute -- cuts the open session, so a stretch in one
// window is stored as a chain of rows that touch. Joins such chains back into
// the sessions the user actually had: same app, title and category, no more
// than kJoinGapMs apart. Takes and returns them newest first, the order the
// repository gives them in.
QList<Activity> joinContiguous(const QList<Activity> &sessions);

} // namespace chronexa::activity
