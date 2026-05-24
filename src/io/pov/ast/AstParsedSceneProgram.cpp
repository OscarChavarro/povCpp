#include "io/pov/ast/AstParsedSceneProgram.h"

AstParsedSceneProgram::AstParsedSceneProgram()
{
    scene = nullptr;
    legacyFrame.viewPoint.initializeDefaults();
    legacyFrame.Light_Sources = nullptr;
    legacyFrame.Objects = nullptr;
    legacyFrame.atmosphereIor = 1.0;
    legacyFrame.antialiasThreshold = 0.0;
    legacyFrame.fogDistance = 0.0;
    legacyFrame.fogColour.Red = 0.0;
    legacyFrame.fogColour.Green = 0.0;
    legacyFrame.fogColour.Blue = 0.0;
    legacyFrame.fogColour.Alpha = 0.0;
    hasCamera = false;
    hasFog = false;
}
