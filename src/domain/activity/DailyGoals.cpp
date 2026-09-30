#include "DailyGoals.hpp"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

constexpr auto kCategoryKey = "category";
constexpr auto kKindKey = "kind";
constexpr auto kMinutesKey = "minutes";
constexpr auto kDaysKey = "days";

constexpr auto kLimitKey = "limit";
constexpr auto kTargetKey = "target";

} // namespace

namespace chronexa::activity {

QString goalKindKey(GoalKind kind) {
  return QLatin1String(kind == GoalKind::Target ? kTargetKey : kLimitKey);
}

GoalKind goalKindFromKey(const QString &key) {
  return key == QLatin1String(kTargetKey) ? GoalKind::Target : GoalKind::Limit;
}

bool DailyGoal::operator==(const DailyGoal &other) const {
  return category == other.category && kind == other.kind &&
         minutes == other.minutes && days == other.days;
}

bool DailyGoal::operator!=(const DailyGoal &other) const {
  return !(*this == other);
}

bool DailyGoal::isValid() const {
  return !category.trimmed().isEmpty() && minutes > 0 &&
         minutes <= kMaxMinutes &&
         (days & TrackingSchedule::kEveryDay) != TrackingSchedule::kNoDays;
}

bool DailyGoal::appliesOn(const QDate &date) const {
  if (!date.isValid()) {
    return false;
  }
  TrackingSchedule mask;
  mask.days = days;
  return mask.includesDay(date.dayOfWeek());
}

qint64 DailyGoal::thresholdSeconds() const {
  return static_cast<qint64>(minutes) * 60;
}

bool GoalProgress::operator==(const GoalProgress &other) const {
  return goal == other.goal && seconds == other.seconds;
}

bool GoalProgress::operator!=(const GoalProgress &other) const {
  return !(*this == other);
}

GoalState GoalProgress::state() const {
  const qint64 threshold = goal.thresholdSeconds();
  if (goal.kind == GoalKind::Limit) {
    // "At most an hour": exactly an hour is still within it.
    return seconds > threshold ? GoalState::Exceeded : GoalState::Within;
  }
  return seconds >= threshold ? GoalState::Reached : GoalState::Short;
}

bool GoalProgress::crossed() const {
  const GoalState current = state();
  return current == GoalState::Exceeded || current == GoalState::Reached;
}

double GoalProgress::fraction() const {
  const qint64 threshold = goal.thresholdSeconds();
  if (threshold <= 0) {
    return 0.0;
  }
  return static_cast<double>(qMax<qint64>(0, seconds)) /
         static_cast<double>(threshold);
}

qint64 GoalProgress::remainingSeconds() const {
  return qMax<qint64>(0, goal.thresholdSeconds() - seconds);
}

qint64 GoalProgress::overSeconds() const {
  return qMax<qint64>(0, seconds - goal.thresholdSeconds());
}

QList<GoalProgress> goalProgress(const QList<DailyGoal> &goals,
                                 const QDate &day,
                                 const QList<CategoryTotal> &totals) {
  QHash<QString, qint64> secondsByCategory;
  for (const CategoryTotal &total : totals) {
    secondsByCategory[total.category] += total.seconds;
  }

  QList<GoalProgress> result;
  for (const DailyGoal &goal : goals) {
    if (!goal.isValid() || !goal.appliesOn(day)) {
      continue;
    }
    result.append({goal, secondsByCategory.value(goal.category.trimmed(), 0)});
  }
  return result;
}

QString serializeDailyGoals(const QList<DailyGoal> &goals) {
  QJsonArray array;
  for (const DailyGoal &goal : goals) {
    QJsonObject object;
    object.insert(QLatin1String(kCategoryKey), goal.category);
    object.insert(QLatin1String(kKindKey), goalKindKey(goal.kind));
    object.insert(QLatin1String(kMinutesKey), goal.minutes);
    object.insert(QLatin1String(kDaysKey), goal.days);
    array.append(object);
  }
  return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QList<DailyGoal> parseDailyGoals(const QString &json) {
  QList<DailyGoal> goals;
  const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8());
  if (!document.isArray()) {
    return goals;
  }

  const QJsonArray array = document.array();
  for (const QJsonValue &value : array) {
    if (!value.isObject()) {
      continue;
    }
    const QJsonObject object = value.toObject();
    DailyGoal goal;
    goal.category = object.value(QLatin1String(kCategoryKey)).toString();
    goal.kind =
        goalKindFromKey(object.value(QLatin1String(kKindKey)).toString());
    goal.minutes =
        qBound(0, object.value(QLatin1String(kMinutesKey)).toInt(goal.minutes),
               DailyGoal::kMaxMinutes);
    goal.days = object.value(QLatin1String(kDaysKey)).toInt(goal.days) &
                TrackingSchedule::kEveryDay;
    goals.append(goal);
  }
  return goals;
}

} // namespace chronexa::activity
