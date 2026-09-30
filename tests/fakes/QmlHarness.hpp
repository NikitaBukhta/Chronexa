#pragma once

// Helpers for tests that load the real QML offscreen. Header-only, like
// Fakes.hpp; the including target defines CHRONEXA_QML_IMPORT_DIR (the module
// directory the Chronexa target writes) and CHRONEXA_QML_SOURCE_DIR.

#include <QQmlAbstractUrlInterceptor>
#include <QQuickItem>
#include <QUrl>

#include <functional>

namespace chronexa::activity::testing {

// The module's qmldir prefers the copies compiled into Chronexa.exe, which a
// test executable does not have; serve the sources instead.
class SourceModule : public QQmlAbstractUrlInterceptor {
public:
  QUrl intercept(const QUrl &url, DataType) override {
    const QString prefix = QStringLiteral("/qt/qml/Chronexa/");
    if (url.scheme() != QStringLiteral("qrc") ||
        !url.path().startsWith(prefix)) {
      return url;
    }
    const QString file = url.path().mid(prefix.size());
    // The qmldir is generated; everything else is served from the sources.
    return QUrl::fromLocalFile(
        file == QStringLiteral("qmldir")
            ? QStringLiteral(CHRONEXA_QML_IMPORT_DIR "/Chronexa/qmldir")
            : QStringLiteral(CHRONEXA_QML_SOURCE_DIR "/") + file);
  }
};

inline QQuickItem *findItem(QQuickItem *root,
                            const std::function<bool(QQuickItem *)> &matches) {
  if (root == nullptr) {
    return nullptr;
  }
  if (matches(root)) {
    return root;
  }
  const QList<QQuickItem *> children = root->childItems();
  for (QQuickItem *child : children) {
    if (QQuickItem *found = findItem(child, matches)) {
      return found;
    }
  }
  return nullptr;
}

inline QQuickItem *byName(QQuickItem *root, const QString &name) {
  return findItem(root, [&](QQuickItem *item) {
    return item->objectName() == name && item->isVisible();
  });
}

// A visible button whose label is exactly `text`.
inline QQuickItem *button(QQuickItem *root, const QString &text) {
  return findItem(root, [&](QQuickItem *item) {
    return item->inherits("QQuickAbstractButton") && item->isVisible() &&
           item->property("text").toString() == text;
  });
}

inline QQuickItem *textField(QQuickItem *root) {
  return findItem(root, [](QQuickItem *item) {
    return item->inherits("QQuickTextField");
  });
}

// A visible text item showing exactly `text`.
inline QQuickItem *label(QQuickItem *root, const QString &text) {
  return findItem(root, [&](QQuickItem *item) {
    return item->inherits("QQuickText") && item->isVisible() &&
           item->property("text").toString() == text;
  });
}

} // namespace chronexa::activity::testing
