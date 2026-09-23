#pragma once

#include "domain/activity/ActivityStats.hpp"

#include <QAbstractListModel>
#include <QList>

namespace chronexa::activity {

class TimeBucketModel : public QAbstractListModel {
  Q_OBJECT
  Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
  Q_PROPERTY(qint64 peakSeconds READ peakSeconds NOTIFY countChanged)

public:
  enum Role {
    StartRole = Qt::UserRole + 1,
    EndRole,
    SecondsRole,
    DurationTextRole,
    LabelRole,
    DescriptionRole,
    ShareRole,
    IsCurrentRole,
  };

  explicit TimeBucketModel(QObject *parent = nullptr);

  int rowCount(const QModelIndex &parent = QModelIndex()) const override;
  QVariant data(const QModelIndex &index,
                int role = Qt::DisplayRole) const override;
  QHash<int, QByteArray> roleNames() const override;

  qint64 peakSeconds() const;

  void setBuckets(QList<BucketTotal> buckets, Granularity granularity);

signals:
  void countChanged();

private:
  QList<BucketTotal> _buckets;
  Granularity _granularity = Granularity::Day;
  qint64 _peakSeconds = 0;
};

} // namespace chronexa::activity
