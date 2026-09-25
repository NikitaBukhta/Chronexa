#include "ActivityQueryController.hpp"

#include "ActivityFormat.hpp"

#include <QLoggingCategory>
#include <QTimer>

namespace {

Q_LOGGING_CATEGORY(lcQuery, "chronexa.activity.query")

constexpr int kColorSlots = 8;

constexpr int kSessionLimit = 1000;

constexpr int kRefreshDebounceMs = 200;

// Polled rather than armed for the next midnight: a machine that sleeps
// through that edge would never fire an edge timer.
constexpr int kRolloverCheckMs = 60 * 1000;

} // namespace

namespace chronexa::activity {

ActivityQueryController::ActivityQueryController(
    ActivityQueryService &queryService, QObject *parent)
    : QObject(parent), _queryService(queryService),
      _buckets(new TimeBucketModel(this)), _appTotals(new AppTotalsModel(this)),
      _categoryTotals(new AppTotalsModel(this)),
      _sessions(new UserActivityModel(this)) {
  _refreshTimer = new QTimer(this);
  _refreshTimer->setSingleShot(true);
  _refreshTimer->setInterval(kRefreshDebounceMs);
  connect(_refreshTimer, &QTimer::timeout, this,
          &ActivityQueryController::refresh);

  auto *rolloverTimer = new QTimer(this);
  rolloverTimer->setInterval(kRolloverCheckMs);
  rolloverTimer->setTimerType(Qt::CoarseTimer);
  connect(rolloverTimer, &QTimer::timeout, this,
          &ActivityQueryController::checkDayRollover);
  rolloverTimer->start();

  applyPreset(QStringLiteral("today"));
}

QDateTime ActivityQueryController::rangeFrom() const { return _from; }
QDateTime ActivityQueryController::rangeTo() const { return _to; }

QString ActivityQueryController::rangeLabel() const {
  return format::rangeLabel(_from, _to);
}

QString ActivityQueryController::preset() const { return _preset; }

QDate ActivityQueryController::selectedDay() const { return _from.date(); }

bool ActivityQueryController::singleDay() const {
  return _from.isValid() && _to.isValid() &&
         _from == _from.date().startOfDay() &&
         _to == _from.date().addDays(1).startOfDay();
}

bool ActivityQueryController::canShiftForward() const {
  return _to.isValid() && _to <= QDateTime::currentDateTime();
}

QString ActivityQueryController::granularity() const {
  return granularityKey(effectiveGranularity());
}

void ActivityQueryController::setGranularity(const QString &key) {
  const QString normalized = granularityKey(granularityFromKey(key));
  if (normalized == _granularityOverride) {
    return;
  }
  _granularityOverride = normalized;
  emit granularityChanged();
  refresh();
}

bool ActivityQueryController::granularityAuto() const {
  return _granularityOverride.isEmpty();
}

void ActivityQueryController::useAutoGranularity() {
  if (_granularityOverride.isEmpty()) {
    return;
  }
  _granularityOverride.clear();
  emit granularityChanged();
  refresh();
}

Granularity ActivityQueryController::effectiveGranularity() const {
  if (!_granularityOverride.isEmpty()) {
    return granularityFromKey(_granularityOverride);
  }
  return suggestGranularity(_from, _to);
}

qint64 ActivityQueryController::totalSeconds() const {
  return _stats.totalSeconds;
}
int ActivityQueryController::sessionCount() const {
  return _stats.sessionCount;
}
int ActivityQueryController::appCount() const { return _stats.appCount; }
qint64 ActivityQueryController::longestSessionSeconds() const {
  return _stats.longestSessionSeconds;
}
QString ActivityQueryController::longestSessionApp() const {
  return _stats.longestSessionApp;
}
QDateTime ActivityQueryController::firstActivity() const {
  return _stats.firstActivity;
}
QDateTime ActivityQueryController::lastActivity() const {
  return _stats.lastActivity;
}
QString ActivityQueryController::topAppName() const { return _topAppName; }
qint64 ActivityQueryController::topAppSeconds() const { return _topAppSeconds; }
int ActivityQueryController::activeDayCount() const { return _activeDayCount; }
bool ActivityQueryController::isEmpty() const {
  return _stats.sessionCount == 0;
}
bool ActivityQueryController::hasCategoryRules() const {
  return !_queryService.categoryRules().isEmpty();
}
int ActivityQueryController::categoryCount() const { return _categoryCount; }
QStringList ActivityQueryController::categoryNames() const {
  return _queryService.categoryRules().categoryNames();
}

int ActivityQueryController::dayCount() const {
  if (!_from.isValid() || !_to.isValid() || _from >= _to) {
    return 0;
  }
  return static_cast<int>(_from.date().daysTo(_to.addMSecs(-1).date())) + 1;
}

qint64 ActivityQueryController::dailyAverageSeconds() const {
  return _activeDayCount > 0 ? _stats.totalSeconds / _activeDayCount : 0;
}

TimeBucketModel *ActivityQueryController::buckets() const { return _buckets; }
AppTotalsModel *ActivityQueryController::appTotals() const {
  return _appTotals;
}
AppTotalsModel *ActivityQueryController::categoryTotals() const {
  return _categoryTotals;
}
UserActivityModel *ActivityQueryController::sessions() const {
  return _sessions;
}

int ActivityQueryController::colorSlot(const QString &appName) const {
  return _colorOrder.indexOf(appName);
}

void ActivityQueryController::applyPreset(const QString &preset) {
  const QDateTime now = QDateTime::currentDateTime();
  const QDate today = now.date();

  if (preset == QStringLiteral("today")) {
    setRangeInternal(today.startOfDay(), today.addDays(1).startOfDay(), preset);
  } else if (preset == QStringLiteral("yesterday")) {
    setRangeInternal(today.addDays(-1).startOfDay(), today.startOfDay(),
                     preset);
  } else if (preset == QStringLiteral("last24h")) {
    const QDateTime edge =
        alignToGranularity(now, Granularity::Hour).addSecs(60 * 60);
    setRangeInternal(edge.addSecs(-24 * 60 * 60), edge, preset);
  } else if (preset == QStringLiteral("last7days")) {
    setRangeInternal(today.addDays(-6).startOfDay(),
                     today.addDays(1).startOfDay(), preset);
  } else if (preset == QStringLiteral("last30days")) {
    setRangeInternal(today.addDays(-29).startOfDay(),
                     today.addDays(1).startOfDay(), preset);
  } else if (preset == QStringLiteral("thisweek")) {
    setRangeInternal(today.addDays(-(today.dayOfWeek() - 1)).startOfDay(),
                     today.addDays(1).startOfDay(), preset);
  } else if (preset == QStringLiteral("thismonth")) {
    setRangeInternal(QDate(today.year(), today.month(), 1).startOfDay(),
                     today.addDays(1).startOfDay(), preset);
  } else if (preset == QStringLiteral("alltime")) {
    const auto bounds = _queryService.bounds();
    const QDate first = bounds.first.isValid() ? bounds.first.date() : today;
    setRangeInternal(first.startOfDay(), today.addDays(1).startOfDay(), preset);
  } else {
    qCWarning(lcQuery) << "Unknown preset" << preset << "-- keeping range";
  }
}

void ActivityQueryController::setRange(const QDateTime &from,
                                       const QDateTime &to) {
  if (!from.isValid() || !to.isValid()) {
    return;
  }
  const QDateTime first = qMin(from, to);
  const QDateTime last = qMax(from, to);
  if (first == last) {
    return;
  }
  setRangeInternal(first, last, QStringLiteral("custom"));
}

void ActivityQueryController::setDayRange(const QDate &first,
                                          const QDate &last) {
  if (!first.isValid() || !last.isValid()) {
    return;
  }
  const QDate from = qMin(first, last);
  const QDate to = qMax(first, last);
  setRangeInternal(from.startOfDay(), to.addDays(1).startOfDay(),
                   from == to && from == QDate::currentDate()
                       ? QStringLiteral("today")
                       : QStringLiteral("custom"));
}

void ActivityQueryController::selectDay(const QDate &day) {
  if (!day.isValid()) {
    return;
  }
  setRangeInternal(day.startOfDay(), day.addDays(1).startOfDay(),
                   day == QDate::currentDate() ? QStringLiteral("today")
                   : day == QDate::currentDate().addDays(-1)
                       ? QStringLiteral("yesterday")
                       : QStringLiteral("custom"));
}

void ActivityQueryController::shiftRange(int steps) {
  if (steps == 0 || !_from.isValid() || !_to.isValid()) {
    return;
  }

  const qint64 wholeDays = _from.date().daysTo(_to.date());
  const bool alignedToDays = _from == _from.date().startOfDay() &&
                             _to == _to.date().startOfDay() && wholeDays > 0;

  if (alignedToDays) {
    const QDate from = _from.date().addDays(wholeDays * steps);
    const QDate to = _to.date().addDays(wholeDays * steps);
    setRangeInternal(from.startOfDay(), to.startOfDay(),
                     wholeDays == 1 && from == QDate::currentDate()
                         ? QStringLiteral("today")
                         : QStringLiteral("custom"));
    return;
  }

  const qint64 span = _from.msecsTo(_to);
  setRangeInternal(_from.addMSecs(span * steps), _to.addMSecs(span * steps),
                   QStringLiteral("custom"));
}

void ActivityQueryController::setRangeInternal(const QDateTime &from,
                                               const QDateTime &to,
                                               const QString &preset) {
  const bool granularityFollowed = _granularityOverride.isEmpty();
  const Granularity before = effectiveGranularity();

  _from = from;
  _to = to;
  _preset = preset;
  _presetDay = QDate::currentDate();

  emit rangeChanged();
  if (granularityFollowed && effectiveGranularity() != before) {
    emit granularityChanged();
  }
  refresh();
}

void ActivityQueryController::checkDayRollover() {
  // Every preset but "custom" is relative to now, and the range behind it was
  // resolved once when it was applied. The app is built to stay open for days
  // (auto-start, all-day tracking), so without this the day page keeps
  // yesterday's window after midnight and the new day's sessions never show up.
  if (_preset.isEmpty() || _preset == QStringLiteral("custom") ||
      _presetDay == QDate::currentDate()) {
    return;
  }

  // Copied: applyPreset() ends up assigning to _preset, which would otherwise
  // be the very string it was handed a reference to.
  const QString preset = _preset;
  qCInfo(lcQuery) << "Day rolled over -- re-applying preset" << preset;
  applyPreset(preset);
}

void ActivityQueryController::refreshLater() { _refreshTimer->start(); }

void ActivityQueryController::retranslate() {
  // QQmlEngine::retranslate() only re-evaluates qsTr() bindings. Labels built
  // from the range -- day names, month titles, the range label -- are locale
  // formatted in C++ or JS and only re-run when the range is announced again.
  emit rangeChanged();
  refresh();
}

void ActivityQueryController::refresh() {
  _refreshTimer->stop();

  if (!_from.isValid() || !_to.isValid() || _from >= _to) {
    return;
  }

  const Granularity granularity = effectiveGranularity();
  _colorOrder = _queryService.rankedAppNames(kColorSlots);

  _stats = _queryService.stats(_from, _to);

  QList<AppTotal> totals = _queryService.appTotals(_from, _to);
  _topAppName = totals.isEmpty() ? QString() : totals.first().appName;
  _topAppSeconds = totals.isEmpty() ? 0 : totals.first().seconds;

  QList<BucketTotal> dayBuckets =
      _queryService.buckets(_from, _to, Granularity::Day);

  _activeDayCount = 0;
  for (const BucketTotal &day : dayBuckets) {
    if (day.seconds > 0) {
      ++_activeDayCount;
    }
  }

  _appTotals->setColorOrder(_colorOrder);
  _appTotals->setTotals(std::move(totals));

  _buckets->setBuckets(granularity == Granularity::Day
                           ? std::move(dayBuckets)
                           : _queryService.buckets(_from, _to, granularity),
                       granularity);

  _sessions->setColorOrder(_colorOrder);
  QList<Activity> sessions = _queryService.sessions(_from, _to, kSessionLimit);
  QStringList sessionCategories;
  sessionCategories.reserve(sessions.size());
  for (const Activity &session : std::as_const(sessions)) {
    sessionCategories.append(_queryService.categoryOf(session));
  }
  _sessions->setActivities(std::move(sessions), std::move(sessionCategories));

  // Shown through the same model as applications: a category row is a name,
  // a time and a share. Colours follow rule order, so a category keeps its
  // colour on every page; the unclaimed rest stays neutral.
  QList<CategoryTotal> categories = _queryService.categoryTotals(_from, _to);
  QList<AppTotal> categoryRows;
  categoryRows.reserve(categories.size());
  _categoryCount = 0;
  for (CategoryTotal &category : categories) {
    if (category.category.isEmpty()) {
      category.category = tr("Uncategorized");
    } else {
      ++_categoryCount;
    }
    categoryRows.append({std::move(category.category), category.seconds,
                         category.sessionCount});
  }
  _categoryTotals->setColorOrder(_queryService.categoryRules().categoryNames());
  _categoryTotals->setTotals(std::move(categoryRows));

  emit dataChanged();
}

} // namespace chronexa::activity
