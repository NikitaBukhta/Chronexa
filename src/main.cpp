#include "core/AppInitializer.hpp"

int main(int argc, char *argv[]) {
  QGuiApplication app(argc, argv);

  chronexa::core::AppInitializer initializer(app);
  return initializer.run();
}
