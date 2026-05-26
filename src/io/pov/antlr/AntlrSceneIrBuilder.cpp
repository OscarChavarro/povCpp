#include "io/pov/antlr/AntlrSceneIrBuilder.h"

#include "io/pov/antlr/AntlrSceneIr.h"

AntlrSceneIrProgram *
AntlrSceneIrBuilder::buildEmptyProgram()
{
    return new AntlrSceneIrProgram();
}
