#include "vsdk/toolkit/common/logging/Logger.h"
#include "common/RenderRuntimeState.h"

RenderRuntimeState *RenderRuntimeState::sActive = nullptr;

void
RenderRuntimeState::reset()
{
    maxTraceLevelValue = 5.0;
    stopFlagValue = 0;
}
