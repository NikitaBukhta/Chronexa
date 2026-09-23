#include "TranslationManager.hpp"

#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QLoggingCategory>
#include <QQmlEngine>
#include <QTranslator>

namespace {

Q_LOGGING_CATEGORY(lcI18n, "chronexa.core.i18n")

constexpr auto kCatalogueDir = ":/i18n";
constexpr auto kCataloguePrefix = "chronexa_";

constexpr auto kSourceLanguage = "en";

QString cataloguePath(const QString &code) {
  return QStringLiteral("%1/%2%3.qm")
      .arg(QLatin1String(kCatalogueDir), QLatin1String(kCataloguePrefix), code);
}

} // namespace

namespace chronexa::core {

TranslationManager::TranslationManager(QQmlEngine *engine, QObject *parent)
    : QObject(parent), _engine(engine) {}

TranslationManager::~TranslationManager() {
  if (_translator) {
    QCoreApplication::removeTranslator(_translator.get());
  }
}

QString TranslationManager::resolvedLanguage() const { return _resolved; }

QString TranslationManager::resolve(const QString &language) const {
  QString code = language;
  if (code.isEmpty() || code == QStringLiteral("system")) {
    code = QLocale::system().name();
  }

  if (code == QLatin1String(kSourceLanguage) ||
      QFile::exists(cataloguePath(code))) {
    return code;
  }

  const QString base = code.section(QLatin1Char('_'), 0, 0);
  if (base != code && (base == QLatin1String(kSourceLanguage) ||
                       QFile::exists(cataloguePath(base)))) {
    return base;
  }

  return QLatin1String(kSourceLanguage);
}

QString TranslationManager::apply(const QString &language) {
  const QString code = resolve(language);

  if (_translator) {
    QCoreApplication::removeTranslator(_translator.get());
    _translator.reset();
  }

  if (code != QLatin1String(kSourceLanguage)) {
    auto translator = std::make_unique<QTranslator>();
    if (translator->load(cataloguePath(code))) {
      QCoreApplication::installTranslator(translator.get());
      _translator = std::move(translator);
    } else {
      qCWarning(lcI18n) << "Could not load" << cataloguePath(code)
                        << "-- falling back to the source language";
    }
  }

  QLocale::setDefault(QLocale(code));

  _resolved = code;

  // Swapping the translator alone changes nothing already on screen; the QML
  // engine has to re-evaluate every qsTr() binding.
  if (_engine != nullptr) {
    _engine->retranslate();
  }

  qCInfo(lcI18n) << "Language" << language << "applied as" << code;
  emit languageApplied();
  return code;
}

} // namespace chronexa::core
