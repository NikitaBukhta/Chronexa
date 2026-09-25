#include "Activity.hpp"

namespace chronexa::activity {

bool Activity::isValid() const {
  return !appName.isEmpty() && startedOn.isValid() && endedOn.isValid() &&
         endedOn > startedOn;
}

qint64 Activity::durationSeconds() const {
  if (!startedOn.isValid() || !endedOn.isValid()) {
    return 0;
  }
  return qMax<qint64>(0, startedOn.secsTo(endedOn));
}

SessionEdit SessionEdit::normalized() const {
  SessionEdit result;
  result.appName = appName.trimmed();
  // Never null: a null title binds as SQL NULL, which the column rejects.
  result.title = title.trimmed();
  if (result.title.isNull()) {
    result.title = QStringLiteral("");
  }
  if (category) {
    result.category = category->trimmed();
  }
  return result;
}

bool SessionEdit::isValid() const { return !appName.trimmed().isEmpty(); }

QList<Activity> joinContiguous(const QList<Activity> &sessions) {
  QList<Activity> joined;
  joined.reserve(sessions.size());
  for (const Activity &session : sessions) {
    if (!joined.isEmpty()) {
      Activity &newer = joined.last();
      const qint64 gap = session.endedOn.msecsTo(newer.startedOn);
      if (newer.appName == session.appName && newer.title == session.title &&
          newer.category == session.category && gap >= -kJoinGapMs &&
          gap <= kJoinGapMs) {
        newer.startedOn = qMin(newer.startedOn, session.startedOn);
        newer.endedOn = qMax(newer.endedOn, session.endedOn);
        continue;
      }
    }
    joined.append(session);
  }
  return joined;
}

} // namespace chronexa::activity
