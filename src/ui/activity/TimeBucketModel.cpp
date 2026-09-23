#include "TimeBucketModel.hpp"

#include "ActivityFormat.hpp"

#include <QDateTime>

namespace chronexa::activity {

TimeBucketModel::TimeBucketModel(QObject *parent)
    : QAbstractListModel(parent) {}

int TimeBucketModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return _buckets.size();
}

qint64 TimeBucketModel::peakSeconds() const { return _peakSeconds; }

QVariant TimeBucketModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid()) {
    return {};
  }
  const int row = index.row();
  if (row < 0 || row >= _buckets.size()) {
    return {};
  }

  const BucketTotal &bucket = _buckets.at(row);
  switch (role) {
  case StartRole:
    return bucket.start;
  case EndRole:
    return bucket.end;
  case SecondsRole:
  case Qt::DisplayRole:
    return bucket.seconds;
  case DurationTextRole:
    return format::duration(bucket.seconds);
  case LabelRole:
    return format::bucketLabel(bucket, _granularity);
  case DescriptionRole:
    return format::bucketDescription(bucket, _granularity);
  case ShareRole:
    return _peakSeconds > 0 ? static_cast<double>(bucket.seconds) / _peakSeconds
                            : 0.0;
  case IsCurrentRole: {
    const QDateTime now = QDateTime::currentDateTime();
    return bucket.start <= now && now < bucket.end;
  }
  default:
    return {};
  }
}

QHash<int, QByteArray> TimeBucketModel::roleNames() const {
  return {
      {StartRole, "start"},     {EndRole, "end"},
      {SecondsRole, "seconds"}, {DurationTextRole, "durationText"},
      {LabelRole, "label"},     {DescriptionRole, "description"},
      {ShareRole, "share"},     {IsCurrentRole, "isCurrent"},
  };
}

void TimeBucketModel::setBuckets(QList<BucketTotal> buckets,
                                 Granularity granularity) {
  beginResetModel();
  _buckets = std::move(buckets);
  _granularity = granularity;
  _peakSeconds = 0;
  for (const BucketTotal &bucket : _buckets) {
    _peakSeconds = qMax(_peakSeconds, bucket.seconds);
  }
  endResetModel();
  emit countChanged();
}

} // namespace chronexa::activity
