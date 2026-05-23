#include "common/LegacyBoolean.h"
#include "environment/material/RendererConfiguration.h"
#include "common/linealAlgebra/Transformation.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/image/DumpFormat.h"
#include "io/image/GifFormat.h"
#include "io/image/IffFormat.h"
#include "io/image/TargaFormat.h"
#include "io/pov/Parse.h"
#include "io/pov/SceneFrameParser.h"
#include "environment/scene/SceneFrame.h"

#include "environment/camera/Camera.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/surface/parametric/ParametricPatch.h"
#include "environment/geometry/volume/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/volume/HeightField.h"
#include "environment/geometry/volume/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/volume/compound/CSG.h"
#include "environment/geometry/volume/compound/Composite.h"
#include "environment/geometry/volume/polynomial/PolynomialShape.h"
#include "environment/light/Light.h"

extern ReservedWord globalReservedWords[];
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;

extern RenderFrame *parsingFramePtr;
extern Constant constants[MAX_CONSTANTS];
extern int numberOfConstants;
extern int degenerateTriangles;

void
SceneParser::Parse(RenderFrame *framePtr)
{
    SimpleBody *object;
    parsingFramePtr = framePtr;

    degenerateTriangles = FALSE;
    SceneParser::tokenInit();
    SceneParser::frameInit();
    SceneParser::parseFrame();
    for (object = parsingFramePtr->Objects; object != nullptr;
        object = object->nextObject) {
        ParseHelpers::postProcessObject(object);
    }
    if (degenerateTriangles) {
        fprintf(
            stderr, "Degenerate triangles were found and are being ignored.\n");
        /* exit(1); Let's ignore degen tri instead of blowing up. CdW */
    }
}

void
SceneParser::tokenInit()
{
    numberOfConstants = 0;
    /*
       Constants = new Constant[MAX_CONSTANTS];
    */
}

/* Set up the fields in the frame to default values. */
void
SceneParser::frameInit()
{
    Default_Texture = TextureUtils::getTexture();
    parsingFramePtr->viewPoint.initializeDefaults();
    parsingFramePtr->Light_Sources = nullptr;
    parsingFramePtr->Objects = nullptr;
    parsingFramePtr->atmosphereIor = 1.0;
    parsingFramePtr->antialiasThreshold = globalRenderingConfiguration.antialiasThreshold;
    parsingFramePtr->fogDistance = 0.0;
    Color::makeColor(&(parsingFramePtr->fogColour), 0.0, 0.0, 0.0);
}

void
SceneParser::parseFrame()
{
    SceneFrameParser::parseFrame(parsingFramePtr);
}
