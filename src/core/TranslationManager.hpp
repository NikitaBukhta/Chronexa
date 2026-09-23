#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include <memory>

class QQmlEngine;
class QTranslator;

namespace chronexa::core {

class TranslationManager : public QObject {
  Q_OBJECT

public:
  explicit TranslationManager(QQmlEngine *engine, QObject *parent = nullptr);
  ~TranslationManager() override;

  QString apply(const QString &language);

  QString resolvedLanguage() const;

signals:
  void languageApplied();

private:
  QString resolve(const QString &language) const;

  QQmlEngine *_engine;
  std::unique_ptr<QTranslator> _translator;
  QString _resolved;
};

} // namespace chronexa::core
