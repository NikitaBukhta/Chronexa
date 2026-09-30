#include "core/AppInitializer.hpp"

#include <QApplication>

int main(int argc, char *argv[]) {
  // the tray icon is a Qt Widgets class.
  QApplication app(argc, argv);

  chronexa::core::AppInitializer initializer(app);
  return initializer.run();
}
