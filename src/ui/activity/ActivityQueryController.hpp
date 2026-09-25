#pragma once

#include "application/activity/ActivityQueryService.hpp"
#include "domain/activity/ActivityStats.hpp"
#include "ui/activity/AppTotalsModel.hpp"
#include "ui/activity/TimeBucketModel.hpp"
#include "ui/activity/UserActivityModel.hpp"

#include <QDate>
#include <QDateTime>
#include <QObject>
#include <QStringList>

class QTimer;

namespace chronexa::activity {

class ActivityQueryController : public QObject {
  Q_OBJECT

  Q_PROPERTY(QDateTime rangeFrom READ rangeFrom NOTIFY rangeChanged)
  Q_PROPERTY(QDateTime rangeTo READ rangeTo NOTIFY rangeChanged)
  Q_PROPERTY(QString rangeLabel READ rangeLabel NOTIFY rangeChanged)
  Q_PROPERTY(QString preset READ preset NOTIFY rangeChanged)
  Q_PROPERTY(QDate selectedDay READ selectedDay NOTIFY rangeChanged)
  Q_PROPERTY(bool singleDay READ singleDay NOTIFY rangeChanged)
  Q_PROPERTY(bool canShiftForward READ canShiftForward NOTIFY rangeChanged)

  Q_PROPERTY(QString granularity READ granularity WRITE setGranularity NOTIFY
                 granularityChanged)
  Q_PROPERTY(
      bool granularityAuto READ granularityAuto NOTIFY granularityChanged)

  Q_PROPERTY(qint64 totalSeconds READ totalSeconds NOTIFY dataChanged)
  Q_PROPERTY(int sessionCount READ sessionCount NOTIFY dataChanged)
  Q_PROPERTY(int appCount READ appCount NOTIFY dataChanged)
  Q_PROPERTY(qint64 longestSessionSeconds READ longestSessionSeconds NOTIFY
                 dataChanged)
  Q_PROPERTY(
      QString longestSessionApp READ longestSessionApp NOTIFY dataChanged)
  Q_PROPERTY(QDateTime firstActivity READ firstActivity NOTIFY dataChanged)
  Q_PROPERTY(QDateTime lastActivity READ lastActivity NOTIFY dataChanged)
  Q_PROPERTY(QString topAppName READ topAppName NOTIFY dataChanged)
  Q_PROPERTY(qint64 topAppSeconds READ topAppSeconds NOTIFY dataChanged)
  Q_PROPERTY(int dayCount READ dayCount NOTIFY dataChanged)
  Q_PROPERTY(int activeDayCount READ activeDayCount NOTIFY dataChanged)
  Q_PROPERTY(
      qint64 dailyAverageSeconds READ dailyAverageSeconds NOTIFY dataChanged)
  Q_PROPERTY(bool empty READ isEmpty NOTIFY dataChanged)
  Q_PROPERTY(bool hasCategoryRules READ hasCategoryRules NOTIFY dataChanged)
  Q_PROPERTY(int categoryCount READ categoryCount NOTIFY dataChanged)
  Q_PROPERTY(QStringList categoryNames READ categoryNames NOTIFY dataChanged)

  Q_PROPERTY(chronexa::activity::TimeBucketModel *buckets READ buckets CONSTANT)
  Q_PROPERTY(
      chronexa::activity::AppTotalsModel *appTotals READ appTotals CONSTANT)
  Q_PROPERTY(chronexa::activity::AppTotalsModel *categoryTotals READ
                 categoryTotals CONSTANT)
  Q_PROPERTY(
      chronexa::activity::UserActivityModel *sessions READ sessions CONSTANT)

public:
  explicit ActivityQueryController(ActivityQueryService &queryService,
                                   QObject *parent = nullptr);

  [[nodiscard]] QDateTime rangeFrom() const;
  [[nodiscard]] QDateTime rangeTo() const;
  [[nodiscard]] QString rangeLabel() const;
  [[nodiscard]] QString preset() const;
  [[nodiscard]] QDate selectedDay() const;
  [[nodiscard]] bool singleDay() const;
  [[nodiscard]] bool canShiftForward() const;

  [[nodiscard]] QString granularity() const;
  void setGranularity(const QString &key);
  [[nodiscard]] bool granularityAuto() const;

  [[nodiscard]] qint64 totalSeconds() const;
  [[nodiscard]] int sessionCount() const;
  [[nodiscard]] int appCount() const;
  [[nodiscard]] qint64 longestSessionSeconds() const;
  [[nodiscard]] QString longestSessionApp() const;
  [[nodiscard]] QDateTime firstActivity() const;
  [[nodiscard]] QDateTime lastActivity() const;
  [[nodiscard]] QString topAppName() const;
  [[nodiscard]] qint64 topAppSeconds() const;
  [[nodiscard]] int dayCount() const;
  [[nodiscard]] int activeDayCount() const;
  [[nodiscard]] qint64 dailyAverageSeconds() const;
  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] bool hasCategoryRules() const;
  [[nodiscard]] int categoryCount() const;
  [[nodiscard]] QStringList categoryNames() const;

  [[nodiscard]] TimeBucketModel *buckets() const;
  [[nodiscard]] AppTotalsModel *appTotals() const;
  [[nodiscard]] AppTotalsModel *categoryTotals() const;
  [[nodiscard]] UserActivityModel *sessions() const;

  Q_INVOKABLE void applyPreset(const QString &preset);

  Q_INVOKABLE void setRange(const QDateTime &from, const QDateTime &to);

  Q_INVOKABLE void setDayRange(const QDate &first, const QDate &last);

  Q_INVOKABLE void selectDay(const QDate &day);

  Q_INVOKABLE void shiftRange(int steps);

  Q_INVOKABLE void useAutoGranularity();

  Q_INVOKABLE int colorSlot(const QString &appName) const;

  Q_INVOKABLE void refresh();

  Q_INVOKABLE void refreshLater();

  void retranslate();

signals:
  void rangeChanged();
  void granularityChanged();
  void dataChanged();

private:
  void setRangeInternal(const QDateTime &from, const QDateTime &to,
                        const QString &preset);
  [[nodiscard]] Granularity effectiveGranularity() const;
  void checkDayRollover();

private:
  ActivityQueryService &_queryService;

  TimeBucketModel *_buckets = nullptr;
  AppTotalsModel *_appTotals = nullptr;
  AppTotalsModel *_categoryTotals = nullptr;
  UserActivityModel *_sessions = nullptr;
  QTimer *_refreshTimer = nullptr;

  QDateTime _from;
  QDateTime _to;
  QString _preset;
  QDate _presetDay;

  QString _granularityOverride;

  QStringList _colorOrder;
  RangeStats _stats;
  QString _topAppName;
  qint64 _topAppSeconds = 0;
  int _activeDayCount = 0;
  int _categoryCount = 0;
};

} // namespace chronexa::activity
