#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/RenderRuntimeState.h"

void
RenderRuntimeState::reset()
{
    maxTraceLevelValue = 5.0;
    stopFlagValue = 0;
}
