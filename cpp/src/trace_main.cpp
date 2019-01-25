#include <chrono>

#include "optics.h"
#include "context.h"

using namespace IceHalo;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    printf("USAGE: %s <config-file>\n", argv[0]);
    return -1;
  }

  auto start = std::chrono::system_clock::now();

  auto parser = ContextParser::createFileParser(argv[1]);
  auto context = parser->parseSimulationSettings();
  context->applySettings();

  auto t = std::chrono::system_clock::now();
  std::chrono::duration<float, std::ratio<1, 1000> > diff = t - start;
  printf("Initialization: %.2fms\n", diff.count());

  char filename[256];
  for (auto wl : context->getWavelengths()) {
    printf("starting at wavelength: %.1f\n", wl);

    context->setCurrentWavelength(wl);

    auto t0 = std::chrono::system_clock::now();
    Optics::traceRays(context);
    auto t1 = std::chrono::system_clock::now();
    diff = t1 - t0;
    printf("Ray tracing: %.2fms\n", diff.count());

    t0 = std::chrono::system_clock::now();
    std::sprintf(filename, "directions_%.1f_%lli.bin", wl, t0.time_since_epoch().count());
    context->writeFinalDirections(filename);

    t1 = std::chrono::system_clock::now();
    diff = t1 - t0;
    printf("Writing: %.2fms\n", diff.count());
  }
  context->printCrystalInfo();

  auto end = std::chrono::system_clock::now();
  diff = end - start;
  printf("Total: %.3fs\n", diff.count() / 1e3);

  return 0;
}
