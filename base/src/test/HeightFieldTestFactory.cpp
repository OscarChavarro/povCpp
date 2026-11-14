#include <cmath>
#include "HeightFieldTestFactory.h"

HeightFieldSolid HeightFieldTestFactory::createPlane()
{
    HeightFieldSolid solid(5, 5, Vector3Dd(1, 0, 1), Vector3Dd(3, 5, 3));
    for (long z = 0; z < 5; ++z) {
        for (long x = 0; x < 5; ++x) solid.setHeight(x, z, 1.0 + 0.5*x + 0.25*z);
    }
    solid.updateBlocks();
    return solid;
}

HeightFieldSolid HeightFieldTestFactory::createHills()
{
    HeightFieldSolid solid(17, 17, Vector3Dd(1, 0, 1), Vector3Dd(15, 6, 15));
    for (long z = 0; z < 17; ++z) {
        for (long x = 0; x < 17; ++x) {
            solid.setHeight(x, z, 2.5 + 1.5*std::sin(0.7*x)*std::cos(0.45*z) + 0.3*std::sin(1.9*x + 1.3*z));
        }
    }
    solid.updateBlocks();
    return solid;
}
