#include "io/pov/Parse.h"
#include "io/pov/SceneFrameParser.h"
#include "common/FrameConfig.h"
#include "common/Transformation.h"
#include "app/PovApp.h"
#include "common/linealAlgebra/Vector3Dd.h"
#include "io/GifFormat.h"
#include "io/IffFormat.h"
#include "io/TargaFormat.h"
#include "io/DumpFormat.h"
#include "render/RenderEngine.h"

#include "environment/geometry/Bezier.h"
#include "environment/geometry/Blob.h"
#include "environment/geometry/volume/Box.h"
#include "environment/geometry/CSG.h"
#include "environment/geometry/HeightField.h"
#include "environment/light/Light.h"
#include "environment/geometry/Composite.h"
#include "environment/geometry/surface/InfinitePlane.h"
#include "environment/geometry/PolynomialShape.h"
#include "environment/geometry/Quadric.h"
#include "environment/geometry/volume/Sphere.h"
#include "environment/geometry/elements/Triangle.h"
#include "environment/geometry/Viewpoint.h"

extern ReservedWord globalReservedWords[];
extern double antialiasThreshold;
extern int termCounts[MAX_ORDER + 1];
extern TokenStruct globalToken;
extern double maxTraceLevel;
extern char verboseFormat;
extern unsigned int Options;
extern char statFileName[FILE_NAME_LENGTH];

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
         object = object->Next_Object) {
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
    parsingFramePtr->View_Point.initializeDefaults();
    parsingFramePtr->Light_Sources = nullptr;
    parsingFramePtr->Objects = nullptr;
    parsingFramePtr->Atmosphere_IOR = 1.0;
    parsingFramePtr->Antialias_Threshold = antialiasThreshold;
    parsingFramePtr->Fog_Distance = 0.0;
    Color::makeColor(&(parsingFramePtr->Fog_Colour), 0.0, 0.0, 0.0);
}

void
SceneParser::parseFrame()
{
    SceneFrameParser::parseFrame(parsingFramePtr);
}
