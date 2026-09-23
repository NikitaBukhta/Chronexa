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

  QDateTime rangeFrom() const;
  QDateTime rangeTo() const;
  QString rangeLabel() const;
  QString preset() const;
  QDate selectedDay() const;
  bool singleDay() const;
  bool canShiftForward() const;

  QString granularity() const;
  void setGranularity(const QString &key);
  bool granularityAuto() const;

  qint64 totalSeconds() const;
  int sessionCount() const;
  int appCount() const;
  qint64 longestSessionSeconds() const;
  QString longestSessionApp() const;
  QDateTime firstActivity() const;
  QDateTime lastActivity() const;
  QString topAppName() const;
  qint64 topAppSeconds() const;
  int dayCount() const;
  int activeDayCount() const;
  qint64 dailyAverageSeconds() const;
  bool isEmpty() const;
  bool hasCategoryRules() const;
  int categoryCount() const;

  TimeBucketModel *buckets() const;
  AppTotalsModel *appTotals() const;
  AppTotalsModel *categoryTotals() const;
  UserActivityModel *sessions() const;

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
  Granularity effectiveGranularity() const;
  void checkDayRollover();

  ActivityQueryService &_queryService;

  TimeBucketModel *_buckets = nullptr;
  AppTotalsModel *_appTotals = nullptr;
  AppTotalsModel *_categoryTotals = nullptr;
  UserActivityModel *_sessions = nullptr;
  QTimer *_refreshTimer = nullptr;

  QDateTime _from;
  QDateTime _to;
  QString _preset;

  // The day the current preset was resolved against, so a rollover can be
  // detected while the app stays open.
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
