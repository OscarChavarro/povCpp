#include <cstdio>

#include "vsdk/toolkit/common/logging/Logger.h"

#include "common/RenderRuntimeState.h"

static double maxTraceLevelInstance = 5.0;
static volatile int stopFlagInstance = 0;

double &
RenderRuntimeState::maxTraceLevel()
{
    return maxTraceLevelInstance;
}

volatile int &
RenderRuntimeState::stopFlag()
{
    return stopFlagInstance;
}

void
RenderRuntimeState::reset()
{
    maxTraceLevelInstance = 5.0;
    stopFlagInstance = 0;
}
