#include "application.hpp"

int main(int, char **) {
  Application app;
  if (!app.init()) {
    return -1;
  }
  return app.run();
}
