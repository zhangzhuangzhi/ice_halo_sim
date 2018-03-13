#include <vector>
#include <chrono>

#include "geometry.h"
#include "optics.h"
#include "testhelper.h"

int main(int argc, char *argv[])
{
    TestContext context;
    context.setIncDirNum(50000);
    context.param.raysPerDirection = 20;
    context.setSunPosition(-90.0f*Geometry::PI/180.0f, 27.0f*Geometry::PI/180.0f);

    context.oriGen.setAxisOrientation(OrientationGenerator::AxisDistribution::AX_ZENITHAL_GAUSS, 0.5f);
    context.oriGen.setAxisRoll(OrientationGenerator::RollDistribution::ROLL_UNIFORM, 0.0f);

    context.applySettings();

    std::vector<Ray*> rays;

    for (float wl = 400.0f; ;) {
        if (wl > 655) {
            break;
        }
        printf("starting at wavelength: %.1f\n", wl);
        context.setWavelength(wl);

        if (!rays.empty()) {
            for (auto r : rays) {
                delete r;
            }
        }
        rays.clear();

        auto t0 = std::chrono::system_clock::now();
        Optics::traceRays(context.getIncDirNum(), context.getRayDirections(), context.param, context.getGeometry(), rays);
        auto t1 = std::chrono::system_clock::now();
        printf("%.2fms\n", (t1 - t0).count() / 1000.0);

        context.writeFinalDirections(rays);

        wl += 25.0f;
    }

    return 0;
}
