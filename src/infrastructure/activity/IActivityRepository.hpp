#pragma once

#include "domain/activity/Activity.hpp"
#include "domain/activity/ActivityStats.hpp"

#include <QList>
#include <QPair>
#include <QStringList>

#include <optional>

namespace chronexa::activity {

class IActivityRepository {
public:
  using Interval = QPair<qint64, qint64>;

  virtual ~IActivityRepository() = default;

  virtual bool open() = 0;
  virtual bool insertBatch(const QList<Activity> &activities) = 0;
  virtual bool clearAll() = 0;

  // Every distinct (app, title) pair ever recorded; nullopt when the history
  // cannot be read, which is not the same as an empty one.
  virtual std::optional<QList<WindowRef>> windows() const = 0;
  // Deletes the sessions of `remove` and empties the title of `hideTitle`, in
  // one transaction, leaving nothing of the old text in the file. The number
  // of sessions changed, or -1 when nothing could be written.
  virtual int redact(const QList<WindowRef> &remove,
                     const QList<WindowRef> &hideTitle) = 0;

  virtual QList<Activity> sessions(const QDateTime &from, const QDateTime &to,
                                   int limit = -1) const = 0;

  virtual QList<AppTotal> appTotals(const QDateTime &from,
                                    const QDateTime &to) const = 0;

  // Time per distinct (app, title) pair, clipped to the range. The raw
  // material for categories, which are resolved from rules at read time so a
  // rule edit applies to the whole history at once.
  virtual QList<TitleTotal> titleTotals(const QDateTime &from,
                                        const QDateTime &to) const = 0;

  virtual QList<Interval> intervals(const QDateTime &from,
                                    const QDateTime &to) const = 0;

  virtual RangeStats stats(const QDateTime &from,
                           const QDateTime &to) const = 0;

  virtual QPair<QDateTime, QDateTime> bounds() const = 0;
  virtual QStringList rankedAppNames(int limit) const = 0;
};

} // namespace chronexa::activity
