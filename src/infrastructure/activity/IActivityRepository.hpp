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

  [[nodiscard]] virtual std::optional<QList<WindowRef>> windows() const = 0;
  virtual int redact(const QList<WindowRef> &remove,
                     const QList<WindowRef> &hideTitle) = 0;
  virtual int editSessions(const WindowRef &window, const QDateTime &from,
                           const QDateTime &to, const SessionEdit &edit) = 0;
  virtual int cutSessions(const WindowRef &window, const QDateTime &from,
                          const QDateTime &to) = 0;

  [[nodiscard]] virtual QList<Activity> sessions(const QDateTime &from,
                                                 const QDateTime &to,
                                                 int limit = -1) const = 0;
  [[nodiscard]] virtual QList<AppTotal>
  appTotals(const QDateTime &from, const QDateTime &to) const = 0;
  [[nodiscard]] virtual QList<TitleTotal>
  titleTotals(const QDateTime &from, const QDateTime &to) const = 0;
  [[nodiscard]] virtual QList<Interval>
  intervals(const QDateTime &from, const QDateTime &to) const = 0;
  [[nodiscard]] virtual RangeStats stats(const QDateTime &from,
                                         const QDateTime &to) const = 0;
  [[nodiscard]] virtual QPair<QDateTime, QDateTime> bounds() const = 0;
  [[nodiscard]] virtual QStringList rankedAppNames(int limit) const = 0;
};

} // namespace chronexa::activity
