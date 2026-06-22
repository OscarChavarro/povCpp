#include "vsdk/toolkit/common/logging/Logger.h"
#include "io/context/RenderRuntimeState.h"

void
RenderRuntimeState::reset()
{
    maxTraceLevelValue = 5.0;
    stopFlagValue = false;
}
