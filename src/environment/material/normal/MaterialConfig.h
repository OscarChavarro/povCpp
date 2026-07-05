#ifndef __MATERIAL_CONFIG__
#define __MATERIAL_CONFIG__

#include <cstdio>

#include "vsdk/toolkit/common/logging/Logger.h"

class MaterialConfig {
  public:
    static constexpr double BUMP_SMALL_TOLERANCE = 0.001;

    static void print();
};

inline void
MaterialConfig::print()
{
    char buffer[256];

    Logger::reportMessage("MaterialConfig", Logger::WARNING, "", "\nMaterialConfig:");

    snprintf(buffer, sizeof(buffer), "    BUMP_SMALL_TOLERANCE = %.17g", BUMP_SMALL_TOLERANCE);
    Logger::reportMessage("MaterialConfig", Logger::WARNING, "", buffer);
}

#endif
