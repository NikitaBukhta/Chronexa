#include "core/AppInitializer.hpp"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  QGuiApplication::setOrganizationName("Chronexa");
  QGuiApplication::setOrganizationDomain("chronexa.local");
  QGuiApplication::setApplicationName("Chronexa");

  chronexa::core::AppInitializer initializer(app);
  return initializer.run();
}
